#include <stdio.h>
#include "ruxml/parser.hpp"

const char* test_string = "<tag>text</tag>\n<sct/><!--comment-with-hyphens--><gat><inner>gfg</inner></gat>";

void test_lexer1() {
  printf("\n--- test_lexer1\n");

  Parser parser = {};
  parser_init(&parser);
  parser_open_memory(&parser, "test"_str, test_string, 0, strlen(test_string));

  while (true) {
    auto token = get_token(&parser);
    if (token.type == TOK_INVALID) break;
    print_token(token);
  }

  parser_destroy(&parser);
}

void test_lexer2() {
  printf("\n--- test_lexer2\n");
   Parser parser = {};
   parser_init(&parser);
   parser_open_file_mmap(&parser, "test/test4.xml"_str);

   while (true) {
     auto token = get_token(&parser);
     if (token.type == TOK_INVALID) break;
     print_token(token);
   }

   parser_destroy(&parser);
 }

void test_lexer3() {
  printf("\n--- test_lexer3\n");
  Parser parser = {};
  parser_init(&parser);
  parser_open_memory(&parser, "test"_str, test_string, 16, 33);

  while (true) {
    auto token = get_token(&parser);
    if (token.type == TOK_INVALID) break;
    print_token(token);
  }

  parser_destroy(&parser);
}

void test_parser() {
  printf("\n--- test_parser\n");
  Parser parser = {};
  parser_init(&parser);
  parser_open_file_mmap(&parser, "test/test4.xml"_str);

  while (true) {
    auto node = get_node(&parser);
    if (node.type == NODE_INVALID) break;
    print_current_node(&parser);
  }

  parser_destroy(&parser);
}

void test_parser2() {
  printf("\n--- test_parser\n");
  Parser parser = {};
  parser_init(&parser);
  parser_open_file_mmap(&parser, "test/test5.xml"_str);

  while (true) {
    auto node = get_node(&parser);
    if (node.type == NODE_INVALID) break;
    print_current_node(&parser);
  }

  parser_destroy(&parser);
}


struct Writer {
  FILE *file;
  char buffer[1024*1024];
  int left;
  int at;
};

void write_text(Writer *writer, String text) {
  if (text.length > writer->left) {
    fwrite(writer->buffer, writer->at, 1, writer->file);
    writer->at = 0;
    writer->left = array_size(writer->buffer);
    if (text.length > writer->left) {
      fwrite(text.data, text.length, 1, writer->file);
      return;
    }
  }

  memcpy(writer->buffer + writer->at, text.data, text.length);
  writer->left -= text.length;
  writer->at += text.length;
}

void flush(Writer *writer) {
  fwrite(writer->buffer, writer->at, 1, writer->file);
  fflush(writer->file);
}

void test_parser_pretty() {
  Parser parser = {};
  parser_init(&parser);
  parser_open_file_mmap(&parser, "test/test3.xml"_str);

  Writer writer = {};
  writer.file = fopen("test/test3_pretty.xml", "w");
  writer.left = array_size(writer.buffer);

  bool inline_close = false;

  while (true) {
    auto node = get_node(&parser);
    if (node.type == NODE_INVALID) break;
//
//    print_node(node);

    if (node.type == NODE_ELEMENT_BEGIN) {
      if (inline_close) write_text(&writer, "\n"_str);
      for (int64_t i = 0; i < node.depth; i++) write_text(&writer, "  "_str);
      write_text(&writer, "<"_str);
      write_text(&writer, node.text);
      if (node.self_closing) {
        write_text(&writer, "/>"_str);
      } else {
        write_text(&writer, ">"_str);
      }
      inline_close = !node.self_closing;
      if (!inline_close) write_text(&writer, "\n"_str);
    } else if (node.type == NODE_ELEMENT_END) {
      if (!inline_close) {
        for (int64_t i = 0; i < node.depth; i++) write_text(&writer, "  "_str);
      }
      write_text(&writer, "</"_str);
      write_text(&writer, node.text);
      write_text(&writer, ">\n"_str);
      inline_close = false;
    } else if (node.type == NODE_TEXT) {
      if (node.text.length > 16) {
        if (inline_close) write_text(&writer, "\n"_str);
        inline_close = false;
        for (int64_t i = 0; i < node.depth; i++) write_text(&writer, "  "_str);
        write_text(&writer, node.text);
        write_text(&writer, "\n"_str);
      } else {
        write_text(&writer, node.text);
      }
    }
  }

  flush(&writer);
  fclose(writer.file);
  parser_destroy(&parser);
}

int main() {
  test_lexer1();
  test_lexer2();
  test_lexer3();
  test_parser();
  test_parser2();
  test_parser_pretty();
  return 0;
}