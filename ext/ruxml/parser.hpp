#pragma once

#include <cstdint>

#include "str.hpp"

#define TOKEN2(a) (TokenType)(((uint16_t)((a)[1])<<7)+(uint16_t)((a)[0]))

enum TokenType : uint16_t {
  TOK_INVALID = 0,
  TOK_L_ANGLED = '<',
  TOK_R_ANGLED = '>',
  TOK_EQUALS = '=',
  TOK_COLON = ':',
  TOK_SLASH = '/',
  TOK_QUESTION = '?',
  TOK_BANG = '!',
  TOK_HYPHEN = '-',
  TOK_ONE_CHAR = 127,

  TOK_TAG_START_CLOSE = TOKEN2("</"),
  TOK_TAG_XML_START = TOKEN2("<?"),
  TOK_TAG_XML_END = TOKEN2("?>"),
  TOK_TAG_SELF_CLOSE = TOKEN2("/>"),
  TOK_TWO_CHAR = 128 * 127,

  TOK_IDENTIFIER = 20000,
  TOK_VALUE,
  TOK_TEXT,
  TOK_COMMENT_START,
  TOK_COMMENT_END,
};

struct Token {
  TokenType type;
  int64_t line;
  int64_t c0;
  int64_t c1;
  int64_t offset;
  String text;
};

enum LexerAction : uint8_t {
  LA_INVALID,
  LA_ONE_CHAR,
  LA_TWO_CHAR_END,
  LA_WHITESPACE,
  LA_NEWLINE,
  LA_IDENTIFIER,
  LA_VALUE
};

enum LexerMode : uint8_t {
  LM_OUT,
  LM_TAG,
  LM_COMMENT
};

enum ParserSourceType {
  PST_NONE = 0,
  PST_MEMORY,
  PST_MMAP
};

enum NodeType {
  NODE_INVALID,
  NODE_ELEMENT_BEGIN,
  NODE_ELEMENT_END,
  NODE_TEXT,
  NODE_XML_HEADER,
  NODE_COMMENT,

  MAX_NODE_TYPES
};

struct Node {
  NodeType type;
  int64_t line;
  int64_t c0;
  int64_t c1;
  int64_t offset;
  int64_t depth;

  int attribute_count;
  bool self_closing;
  String xml_namespace;
  String text;
};

struct Attribute {
  String xml_namespace;
  String name;
  String value;
};

struct AttributeBlock {
  int count;
  Attribute attributes[32];
  AttributeBlock* next;
};

struct Parser {
  String source;
  ParserSourceType source_type;
  char *buffer;
  int64_t length;

  char *ptr;
  char *end_ptr;

  int64_t line;
  int64_t col;

  bool done;
  bool errored;
  LexerMode mode;

  LexerAction tag_initial_map[256];

  uint8_t identifier_map[256];

  bool has_next_token;
  Token next_token;
  Token token;
  Node node;

  AttributeBlock attribute_block;
  uint64_t current_attribute_index;
  AttributeBlock* current_attribute_block;

  int64_t depth;
};

void parser_init(Parser *parser);
bool parser_open_memory(Parser *parser, String name, const char *memory, int64_t offset = 0, int64_t length = 0);
bool parser_open_file_mmap(Parser *parser, String filename, int64_t offset = 0, int64_t length = 0);
void parser_destroy(Parser *parser);

void print_error_start(Parser *parser, Token token);
bool expect_type(Parser *parser, TokenType type);

Token read_token(Parser *parser); // Internal only: use get_token instead
Node get_node(Parser *parser);
Attribute get_attribute(Parser* parser);

inline Token peek_token(Parser *parser) {
  if (parser->has_next_token) return parser->next_token;
  parser->next_token = read_token(parser);
  parser->has_next_token = true;
  return parser->next_token;
}

inline Token get_token(Parser *parser) {
  if (parser->has_next_token) {
    parser->token = parser->next_token;
    parser->has_next_token = false;
  } else {
    parser->token = read_token(parser);
  }

  if (!parser->token.type) parser->done = true;
  return parser->token;
}

void print_token(Token token);

void print_node(Node node);
void print_attribute(Attribute attr);
void print_current_node(Parser *parser);