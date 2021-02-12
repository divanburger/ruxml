module RUXML
  class Parser

    def get_node
      next_node
      raise ParseError.new("RUXML encountered an error in the XML") if errored
      node
    end

    def each
      while next_node
        yield node
      end
      raise ParseError.new("RUXML encountered an error in the XML") if errored
    end

    def each_node
      while next_node
        yield
      end
      raise ParseError.new("RUXML encountered an error in the XML") if errored
    end

  end
end