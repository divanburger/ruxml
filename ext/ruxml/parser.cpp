#include "parser.hpp"

#include <fcntl.h>
#include <sys/mman.h>
#include <sys/stat.h>

void parser_init(Parser *parser) {
  parser->line = 1;
  parser->col = 1;

  for (int i = 0; i < 128; i++) parser->tag_initial_map[i] = LA_INVALID;
  for (int i = 128; i < 256; i++) parser->tag_initial_map[i] = LA_IDENTIFIER; // UTF-8 characters are allowed
  parser->tag_initial_map[' '] = LA_WHITESPACE;
  parser->tag_initial_map['\r'] = LA_WHITESPACE;
  parser->tag_initial_map['\t'] = LA_WHITESPACE;
  parser->tag_initial_map['\n'] = LA_NEWLINE;
  parser->tag_initial_map['\''] = LA_VALUE;
  parser->tag_initial_map['"'] = LA_VALUE;
  parser->tag_initial_map['<'] = LA_ONE_CHAR;
  parser->tag_initial_map['>'] = LA_ONE_CHAR;
  parser->tag_initial_map['='] = LA_ONE_CHAR;
  parser->tag_initial_map[':'] = LA_ONE_CHAR;
  parser->tag_initial_map['/'] = LA_TWO_CHAR_END;
  parser->tag_initial_map['?'] = LA_TWO_CHAR_END;
  for (int c = 'a'; c <= 'z'; c++) parser->tag_initial_map[c] = LA_IDENTIFIER;
  for (int c = 'A'; c <= 'Z'; c++) parser->tag_initial_map[c] = LA_IDENTIFIER;
  parser->tag_initial_map['_'] = LA_IDENTIFIER;

  for (int i = 0; i < 128; i++) parser->identifier_map[i] = 0;
  for (int i = 128; i < 256; i++) parser->identifier_map[i] = 1; // UTF-8 characters are allowed
  for (int c = 'a'; c <= 'z'; c++) parser->identifier_map[c] = 1;
  for (int c = 'A'; c <= 'Z'; c++) parser->identifier_map[c] = 1;
  for (int c = '0'; c <= '9'; c++) parser->identifier_map[c] = 1;
  parser->identifier_map['-'] = 1;
  parser->identifier_map['_'] = 1;
  parser->identifier_map['.'] = 1;
}

bool parser_open_memory(Parser *parser, String name, const char *memory, int64_t offset, int64_t length) {
  parser->source_type = PST_MEMORY;
  parser->source = name;
  parser->buffer = (char *) memory + offset;
  parser->length = length;
  parser->ptr = parser->buffer;
  parser->end_ptr = parser->buffer + parser->length;
  return true;
}

bool parser_open_file_mmap(Parser *parser, String filename, int64_t offset, int64_t length) {
  parser->source_type = PST_MMAP;
  parser->source = filename;

  char *cpath = str_to_zstr(filename);
  int fd = open(cpath, O_RDWR);
  raw_free(cpath);

  if (fd < 0) {
    fprintf(stderr, "\nCould not open file: %.*s\n", str_prt(filename));
    return false;
  }

  if (length == 0) {
    struct stat stat_result;
    int err = fstat(fd, &stat_result);
    if (err < 0) {
      fprintf(stderr, "\nCould not open file: %.*s\n", str_prt(filename));
      return false;
    }
    parser->length = stat_result.st_size - offset;
  } else {
    parser->length = length;
  }

  parser->buffer = (char *) mmap(nullptr, parser->length, PROT_READ, MAP_SHARED, fd, offset);
  if (parser->buffer == MAP_FAILED) {
    fprintf(stderr, "Could not memory map file: %.*s\n", str_prt(filename));
    return false;
  }

  parser->ptr = parser->buffer;
  parser->end_ptr = parser->buffer + parser->length;

  return true;
}

void parser_destroy(Parser *parser) {
  if (parser->source_type == PST_MMAP) {
    munmap(parser->buffer, parser->length);
  }
}

inline bool scan_value(Parser *parser, Token *token) {
  auto start = parser->ptr;
  auto end = parser->ptr + 1;
  while (end != parser->end_ptr && *end != *start) end++;
  if (end == parser->end_ptr) return false;
  end++;

  int64_t length = end - start;
  token->type = TOK_VALUE;
  token->text.length = length;
  token->text.data = start;

  parser->col += length;
  parser->ptr = end;
  return true;
}

inline void scan_identifier(Parser *parser, Token *token) {
  auto start = parser->ptr;
  auto end = parser->ptr;
  while (end != parser->end_ptr && parser->identifier_map[(unsigned char) *end]) end++;

  int64_t length = end - start;
  token->type = TOK_IDENTIFIER;
  token->text.length = length;
  token->text.data = start;

  parser->col += length;
  parser->ptr = end;
}

void scan_text(Parser *parser, Token *token) {
  auto start = parser->ptr;
  auto end = parser->ptr;
  auto line_start = parser->ptr;
  while (end != parser->end_ptr && *end != '<') {
    if (*end == '\n') {
      line_start = end + 1;
      parser->line++;
      parser->col = 1;
    }
    end++;
  }

  token->type = TOK_TEXT;
  token->text.length = end - start;
  token->text.data = start;

  parser->col += end - line_start;
  parser->ptr = end;
}

bool scan_comment_start(Parser *parser, Token *token) {
  if (parser->ptr == parser->end_ptr || *parser->ptr != '<') return false;
  parser->ptr++;
  if (parser->ptr == parser->end_ptr || *parser->ptr != '!') return false;
  parser->ptr++;
  if (parser->ptr == parser->end_ptr || *parser->ptr != '-') return false;
  parser->ptr++;
  if (parser->ptr == parser->end_ptr || *parser->ptr != '-') return false;
  parser->ptr++;

  token->type = TOK_COMMENT_START;
  parser->col += 4;
  return true;
}

bool scan_comment_end(Parser *parser, Token *token) {
  if (parser->ptr == parser->end_ptr || *parser->ptr != '-') return false;
  parser->ptr++;
  if (parser->ptr == parser->end_ptr || *parser->ptr != '-') return false;
  parser->ptr++;
  if (parser->ptr == parser->end_ptr || *parser->ptr != '>') return false;
  parser->ptr++;

  token->type = TOK_COMMENT_END;
  parser->col += 3;
  return true;
}

void scan_comment(Parser *parser, Token *token) {
  auto start = parser->ptr;
  auto end = parser->ptr;
  auto line_start = parser->ptr;
  while (end != parser->end_ptr) {
    if (*end == '\n') {
      line_start = end + 1;
      parser->line++;
      parser->col = 1;
    }
    if (*end == '-') {
      auto check_end_ptr = end + 1;
      if (check_end_ptr != parser->end_ptr && *check_end_ptr == '-') {
        break;
      }
    }
    end++;
  }

  token->type = TOK_TEXT;
  token->text.length = end - start;
  token->text.data = start;

  parser->col += end - line_start;
  parser->ptr = end;
}

Token read_token(Parser *parser) {
  while (parser->ptr != parser->end_ptr) {
    Token token = {};
    token.line = parser->line;
    token.c0 = parser->col;
    token.offset = parser->ptr - parser->buffer;

    auto c = *parser->ptr;
    if (parser->mode == LM_TAG) {
      auto state = parser->tag_initial_map[(unsigned char) c];
      if (state == LA_WHITESPACE) {
        parser->col++;
        parser->ptr++;
        continue; // Ignore white space in a tag
      } else if (state == LA_NEWLINE) {
        parser->line++;
        parser->col = 1;
        parser->ptr++;
        continue; // Ignore white space in a tag
      } else if (state == LA_ONE_CHAR) {
        token.type = (TokenType) c;
        parser->col++;
        parser->ptr++;
        parser->mode = (token.type == TOK_R_ANGLED) ? LM_OUT : LM_TAG;
      } else if (state == LA_TWO_CHAR_END) {
        auto c2 = *(parser->ptr + 1);
        if (c2 == '>') {
          token.type = TOKEN2(parser->ptr);
          parser->col += 2;
          parser->ptr += 2;
          parser->mode = LM_OUT;
        } else {
          print_error_start(parser, token);
          printf("Didn't expect '%c' to be followed by '%c'\n", c, c2);
          return token;
        }
      } else if (state == LA_IDENTIFIER) {
        scan_identifier(parser, &token);
      } else if (state == LA_VALUE) {
        if (!scan_value(parser, &token)) return token;
      } else {
        print_error_start(parser, token);
        printf("Invalid character '%c'\n", c);
        return token;
      }
    } else if (parser->mode == LM_OUT) {
      if (c == '<') {
        auto c2 = *(parser->ptr + 1);
        if (c2 == '?' || c2 == '/') {
          token.type = TOKEN2(parser->ptr);
          token.c1 = parser->col + 1;
          parser->col += 2;
          parser->ptr += 2;
          parser->mode = LM_TAG;
        } else if (c2 == '!') {
          if (!scan_comment_start(parser, &token)) return token;
          parser->mode = LM_COMMENT;
        } else {
          token.type = TOK_L_ANGLED;
          token.c1 = parser->col;
          parser->col++;
          parser->ptr++;
          parser->mode = LM_TAG;
        }
      } else {
        scan_text(parser, &token);
      }
    } else {
      if (c == '-' && *(parser->ptr + 1) == '-') {
        if (!scan_comment_end(parser, &token)) return token;
        parser->mode = LM_OUT;
      }
      if (!token.type) scan_comment(parser, &token);
    }

    return token;
  }

  Token token = {};
  token.line = parser->line;
  token.c0 = parser->col;
  token.offset = parser->ptr - parser->buffer;
  return token;
}

void print_error_start(Parser *parser, Token token) {
  parser->errored = true;
  printf("%.*s:%li:%li - ", str_prt(parser->source), token.line, token.c0);
}

void print_token_type(TokenType type) {
  if (type == TOK_INVALID) {
    printf("INVALID");
  } else if (type < TOK_ONE_CHAR) {
    printf("'%c'", type);
  } else if (type < TOK_TWO_CHAR) {
    printf("'%c%c'", type & 0x7F, (type >> 7) & 0x7F);
  } else if (type == TOK_COMMENT_START) {
    printf("'<!--'");
  } else if (type == TOK_COMMENT_END) {
    printf("'-->'");
  } else if (type == TOK_IDENTIFIER) {
    printf("identifier");
  } else if (type == TOK_VALUE) {
    printf("value");
  } else if (type == TOK_TEXT) {
    printf("text");
  }
}

void print_token_type(TokenType type, String str) {
  if (type == TOK_INVALID) {
    printf("INVALID");
  } else if (type < TOK_ONE_CHAR) {
    printf("'%c'", type);
  } else if (type < TOK_TWO_CHAR) {
    printf("'%c%c'", type & 0x7F, (type >> 7) & 0x7F);
  } else if (type == TOK_COMMENT_START) {
    printf("'<!--'");
  } else if (type == TOK_COMMENT_END) {
    printf("'-->'");
  } else if (type == TOK_IDENTIFIER) {
    printf("identifier: %.*s", str_prt(str));
  } else if (type == TOK_VALUE) {
    printf("value: %.*s", str_prt(str));
  } else if (type == TOK_TEXT) {
    printf("text: %.*s", str_prt(str));
  }
}

bool expect_type(Parser *parser, TokenType type) {
  Token token = parser->token;
  if (token.type == type) return true;
  print_error_start(parser, token);
  printf("Expected ");
  print_token_type(type);
  if (token.type) {
    printf(" but got ");
    print_token_type(token.type);
  }
  printf("\n");
  return false;
}

void print_token(Token token) {
  printf("%li:%li: ", token.line, token.c0);
  print_token_type(token.type, token.text);
  printf("\n");
}

Node parse_xml_header(Parser *parser) {
  auto start_token = peek_token(parser);
  auto token = get_token(parser);
  if (!expect_type(parser, TOK_IDENTIFIER)) return {};

  Node node = {};
  node.type = NODE_XML_HEADER;
  node.line = start_token.line;
  node.c0 = start_token.c0;
  node.c1 = start_token.c1;
  node.offset = token.offset;
  node.depth = parser->depth;

  while (!parser->done) {
    token = get_token(parser);
    if (token.type == TOK_TAG_XML_END) break;
  }
  expect_type(parser, TOK_TAG_XML_END);

  node.c1 = token.c1;
  return node;
}

Node parse_element_begin(Parser *parser) {
  auto start_token = peek_token(parser);
  auto token = get_token(parser);
  if (!expect_type(parser, TOK_IDENTIFIER)) return {};

  Node node = {};
  node.type = NODE_ELEMENT_BEGIN;
  node.line = start_token.line;
  node.c0 = start_token.c0;
  node.offset = token.offset;
  node.depth = parser->depth;
  node.text = token.text;

  while (!parser->done) {
    token = get_token(parser);
    if (token.type == TOK_TAG_SELF_CLOSE) {
      node.self_closing = true;
      break;
    }
    if (token.type == TOK_R_ANGLED) break;
  }

  node.c1 = token.c1;

  if (!node.self_closing) parser->depth++;
  return node;
}

Node parse_element_end(Parser *parser) {
  auto start_token = peek_token(parser);
  auto token = get_token(parser);
  if (!expect_type(parser, TOK_IDENTIFIER)) return {};

  Node node = {};
  node.type = NODE_ELEMENT_END;
  node.line = start_token.line;
  node.c0 = start_token.c0;
  node.offset = start_token.offset;
  node.depth = parser->depth - 1;
  node.text = token.text;

  while (!parser->done) {
    token = get_token(parser);
    if (token.type == TOK_R_ANGLED) break;
  }

  node.c1 = token.c1;

  parser->depth--;
  return node;
}

Node parse_text(Parser *parser) {
  if (!expect_type(parser, TOK_TEXT)) return {};

  auto token = peek_token(parser);
  Node node = {};
  node.type = NODE_TEXT;
  node.line = token.line;
  node.c0 = token.c0;
  node.c1 = token.c1;
  node.offset = token.offset;
  node.depth = parser->depth;
  node.text = token.text;
  return node;
}

Node parse_comment(Parser *parser) {
  auto start_token = peek_token(parser);
  Node node = {};
  node.line = start_token.line;
  node.c0 = start_token.c0;
  node.offset = start_token.offset;

  auto token = get_token(parser);
  if (!expect_type(parser, TOK_TEXT)) return node;
  node.depth = parser->depth;
  node.text = token.text;

  auto end_token = get_token(parser);
  if (!expect_type(parser, TOK_COMMENT_END)) return node;
  node.type = NODE_COMMENT;
  node.c1 = end_token.c1;
  return node;
}

Node read_node(Parser *parser) {
  if (parser->done || parser->errored) return {};

  auto token = get_token(parser);
  if (token.type == TOK_TAG_XML_START) {
    return parse_xml_header(parser);
  } else if (token.type == TOK_TAG_START_CLOSE) {
    return parse_element_end(parser);
  } else if (token.type == TOK_L_ANGLED) {
    return parse_element_begin(parser);
  } else if (token.type == TOK_COMMENT_START) {
    return parse_comment(parser);
  } else if (token.type == TOK_INVALID) {
    return {};
  } else {
    return parse_text(parser);
  }
}

void print_node(Node node) {
  printf("%li:%li: [%li] ", node.line, node.c0, node.depth);
  if (node.type == NODE_ELEMENT_BEGIN) {
    printf("Begin: '%.*s' %s\n", str_prt(node.text), node.self_closing ? "self-closing" : "");
  } else if (node.type == NODE_ELEMENT_END) {
    printf("End: '%.*s'\n", str_prt(node.text));
  } else if (node.type == NODE_TEXT) {
    printf("Text: '%.*s'\n", str_prt(node.text));
  } else if (node.type == NODE_COMMENT) {
    printf("Comment: '%.*s'\n", str_prt(node.text));
  } else if (node.type == NODE_XML_HEADER) {
    printf("XML header\n");
  }
}