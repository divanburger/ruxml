require 'ruxml'

describe RUXML::Parser, type: :lib do
  it "parses a basic string" do
    subject { described_class.new }

    success = subject.open_string("test", "<tag>text<!--test-comment--></tag>\n<sct/><gat>\n<inner>\n</inner></gat>")
    expect(success).to eq true

    node = subject.get_node
    expect(node.type).to eq :begin
    expect(node.text).to eq 'tag'
    expect(node.self_closing).to eq false
    expect(node.column_start).to eq 1
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :text
    expect(node.text).to eq 'text'
    expect(node.column_start).to eq 6
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :comment
    expect(node.text).to eq 'test-comment'
    expect(node.column_start).to eq 10
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :end
    expect(node.text).to eq 'tag'
    expect(node.column_start).to eq 29
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :text
    expect(node.text).to eq "\n"
    expect(node.column_start).to eq 35
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :begin
    expect(node.text).to eq "sct"
    expect(node.self_closing).to eq true
    expect(node.column_start).to eq 1
    expect(node.line).to eq 2

    node = subject.get_node
    expect(node.type).to eq :begin
    expect(node.text).to eq "gat"
    expect(node.self_closing).to eq false
    expect(node.column_start).to eq 7
    expect(node.line).to eq 2

    node = subject.get_node
    expect(node.type).to eq :text
    expect(node.text).to eq "\n"
    expect(node.column_start).to eq 12
    expect(node.line).to eq 2

    node = subject.get_node
    expect(node.type).to eq :begin
    expect(node.text).to eq "inner"
    expect(node.self_closing).to eq false
    expect(node.column_start).to eq 1
    expect(node.line).to eq 3

    node = subject.get_node
    expect(node.type).to eq :text
    expect(node.text).to eq "\n"
    expect(node.column_start).to eq 8
    expect(node.line).to eq 3

    node = subject.get_node
    expect(node.type).to eq :end
    expect(node.text).to eq "inner"
    expect(node.column_start).to eq 1
    expect(node.line).to eq 4

    node = subject.get_node
    expect(node.type).to eq :end
    expect(node.text).to eq "gat"
    expect(node.column_start).to eq 9
    expect(node.line).to eq 4

    node = subject.get_node
    expect(node.type).to eq :invalid

    expect(subject.done).to eq true
  end

  it "parses a part of a basic string" do
    subject { described_class.new }

    success = subject.open_string("test", "<tag>text<!--test-comment--></tag>\n<sct/><gat>\n<inner>\n</inner></gat>", 35, 11)
    expect(success).to eq true

    node = subject.get_node
    expect(node.type).to eq :begin
    expect(node.text).to eq "sct"
    expect(node.self_closing).to eq true
    expect(node.column_start).to eq 1
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :begin
    expect(node.text).to eq "gat"
    expect(node.self_closing).to eq false
    expect(node.column_start).to eq 7
    expect(node.line).to eq 1

    node = subject.get_node
    expect(node.type).to eq :invalid

    expect(subject.done).to eq true
  end

  describe "parses a file" do
    it "using each" do
      subject { described_class.new }

      success = subject.open_file("./spec/fixtures/text.xml")
      expect(success).to eq true

      begin_counts = 0
      end_counts = 0
      text_counts = 0
      comment_counts = 0
      xml_header_counts = 0
      subject.each do |node|
        expect(node.type).not_to eq :invalid
        begin_counts += 1 if node.type == :begin
        end_counts += 1 if node.type == :end
        text_counts += 1 if node.type == :text
        comment_counts += 1 if node.type == :comment
        xml_header_counts += 1 if node.type == :xml_header
      end

      expect(subject.done).to eq true
      expect(begin_counts).to eq 12
      expect(end_counts).to eq 11
      expect(text_counts).to eq 6
      expect(xml_header_counts).to eq 1
    end

    it "using each_node" do
      subject { described_class.new }

      success = subject.open_file("./spec/fixtures/text.xml")
      expect(success).to eq true

      begin_counts = 0
      end_counts = 0
      text_counts = 0
      comment_counts = 0
      xml_header_counts = 0
      subject.each_node do
        expect(subject.node_type).not_to eq :invalid
        begin_counts += 1 if subject.node_type == :begin
        end_counts += 1 if subject.node_type == :end
        text_counts += 1 if subject.node_type == :text
        comment_counts += 1 if subject.node_type == :comment
        xml_header_counts += 1 if subject.node_type == :xml_header
      end

      expect(subject.done).to eq true
      expect(begin_counts).to eq 12
      expect(end_counts).to eq 11
      expect(text_counts).to eq 6
      expect(xml_header_counts).to eq 1
    end

    it "using next_node" do
      subject { described_class.new }

      success = subject.open_file("./spec/fixtures/text.xml")
      expect(success).to eq true

      begin_counts = 0
      end_counts = 0
      text_counts = 0
      comment_counts = 0
      xml_header_counts = 0
      while subject.next_node
        expect(subject.node_type).not_to eq :invalid
        begin_counts += 1 if subject.node_type == :begin
        end_counts += 1 if subject.node_type == :end
        text_counts += 1 if subject.node_type == :text
        comment_counts += 1 if subject.node_type == :comment
        xml_header_counts += 1 if subject.node_type == :xml_header
      end

      expect(subject.done).to eq true
      expect(begin_counts).to eq 12
      expect(end_counts).to eq 11
      expect(text_counts).to eq 6
      expect(xml_header_counts).to eq 1
    end
  end

  it "errors on broken XML" do
    subject { described_class.new }

    success = subject.open_string("test", "<tag>text<!--test-comment--></t@ag>\n<sct/><gat>\n<inner>\n</inner></gat>")
    expect(success).to eq true

    expect do
      subject.each_node {}
    end.to raise_error(RUXML::ParseError)

    expect(subject.done).to eq true
  end
end
