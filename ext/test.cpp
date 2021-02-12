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
//  parser_open_memory(&parser, "test"_str, test_string, 0, strlen(test_string));

//  int64_t depth = 0;

  while (true) {
    auto node = get_node(&parser);
    if (node.type == NODE_INVALID) break;
    print_node(node);

//    if (node.type == NODE_ELEMENT_BEGIN) {
//      for (int64_t i = 0; i < depth; i++) printf("  ");
//      printf("<%.*s>\n", node.text);
//      depth++;
//    } else if (node.type == NODE_ELEMENT_END) {
//      depth--;
//      for (int64_t i = 0; i < depth; i++) printf("  ");
//      printf("</%.*s>\n", node.text);
//    } else if (node.type == NODE_TEXT) {
//      for (int64_t i = 0; i < depth; i++) printf("  ");
//      printf("%.*s\n", node.text);
//    }
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

  int64_t depth;

  while (true) {
    auto node = get_node(&parser);
    if (node.type == NODE_INVALID) break;
//
//    print_node(node);

    if (node.type == NODE_ELEMENT_BEGIN) {
      for (int64_t i = 0; i < depth; i++) write_text(&writer, "  "_str);
      write_text(&writer, "<"_str);
      if (node.self_closing) {
        write_text(&writer, node.text);
        write_text(&writer, "/>\n"_str);
      } else {
        write_text(&writer, node.text);
        write_text(&writer, ">\n"_str);
        depth++;
      }
    } else if (node.type == NODE_ELEMENT_END) {
      depth--;
      for (int64_t i = 0; i < depth; i++) write_text(&writer, "  "_str);
      write_text(&writer, "</"_str);
      write_text(&writer, node.text);
      write_text(&writer, ">\n"_str);
    } else if (node.type == NODE_TEXT) {
      for (int64_t i = 0; i < depth; i++) write_text(&writer, "  "_str);
      write_text(&writer, node.text);
      write_text(&writer, "\n"_str);
    }
  }

  flush(&writer);
  fclose(writer.file);
  parser_destroy(&parser);
}

int main() {
  test_lexer1();
//  test_lexer2();
//  test_lexer3();
//  test_parser();
//  test_parser_pretty();
  return 0;
}