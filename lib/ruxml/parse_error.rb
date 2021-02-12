module RUXML
  class ParseError < RuntimeError
    attr_reader :message

    def initialize(message)
      @message = message
    end
  end
end