#include "parser.hpp"
#include <ruby/ruby.h>

extern "C"
{

VALUE ruxmlModule;
VALUE ruxmlParser;
VALUE ruxmlNode;

ID node_type_ids[MAX_NODE_TYPES];

//
// Helpers
//

VALUE rbstr_from_str(String str) { return rb_str_export_locale(rb_str_new(str.data, str.length)); }

String str_from_rbstr(VALUE rbstr) { return String{(int) RSTRING_LEN(rbstr), StringValuePtr(rbstr)}; }

//
// Node
//

static Node *Node_instance(VALUE self) {
  return (Node *) RDATA(self)->data;
}

static size_t Node_size(const void *data) {
  return sizeof(Node);
}

static void Node_free(void *data) {
  free(data);
}

rb_data_type_t Node_data_type = {
    "Node",
    {NULL, Node_free, Node_size},
    0, 0,
    RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE Node_allocate(VALUE self) {
  Node *node;
  return TypedData_Make_Struct(self, Node, &Node_data_type, node);
}

static VALUE Node_initialize(VALUE self) {
  Node *node;
  TypedData_Get_Struct(self, Node, &Node_data_type, node);
  *node = Node{};
  return self;
}

static VALUE Node_column_start(VALUE self) {
  auto node = Node_instance(self);
  return INT2NUM(node->c0);
}

static VALUE Node_line(VALUE self) {
  auto node = Node_instance(self);
  return INT2NUM(node->line);
}

static VALUE Node_offset(VALUE self) {
  auto node = Node_instance(self);
  return INT2NUM(node->offset);
}

static VALUE Node_text(VALUE self) {
  auto node = Node_instance(self);
  return rbstr_from_str(node->text);
}

static VALUE Node_type(VALUE self) {
  auto node = Node_instance(self);
  return ID2SYM(node_type_ids[node->type]);
}

static VALUE Node_self_closing(VALUE self) {
  auto node = Node_instance(self);
  return node->self_closing ? Qtrue : Qfalse;
}

//
// Parser
//

static Parser *Parser_instance(VALUE self) {
  return (Parser *) RDATA(self)->data;
}

static size_t Parser_size(const void *data) {
  return sizeof(Parser);
}

static void Parser_free(void *data) {
  parser_destroy((Parser *) data);
  free(data);
}

rb_data_type_t Parser_data_type = {
    "Parser",
    {NULL, Parser_free, Parser_size},
    0, 0,
    RUBY_TYPED_FREE_IMMEDIATELY
};

static VALUE Parser_allocate(VALUE self) {
  Parser *parser;
  return TypedData_Make_Struct(self, Parser, &Parser_data_type, parser);
}

static VALUE Parser_initialize(VALUE self) {
  Parser *parser;
  TypedData_Get_Struct(self, Parser, &Parser_data_type, parser);

  *parser = Parser{};
  parser_init(parser);
  return self;
}

static VALUE Parser_open_string(int argc, VALUE* argv, VALUE self) {
  VALUE name;
  VALUE data;
  VALUE offset;
  VALUE length;
  rb_scan_args(argc, argv, "22", &name, &data, &offset, &length);

  Check_Type(name, T_STRING);
  Check_Type(data, T_STRING);

  int64_t data_offset = 0;
  if (!NIL_P(offset)) {
    Check_Type(offset, T_FIXNUM);
    data_offset = NUM2INT(offset);
  }

  int64_t data_length;
  if (NIL_P(length)) {
    data_length = RSTRING_LEN(data);
  } else {
    Check_Type(length, T_FIXNUM);
    data_length = NUM2INT(length);
  }

  auto parser = Parser_instance(self);
  auto success = parser_open_memory(parser, str_from_rbstr(name), StringValuePtr(data), data_offset, data_length);
  return success ? Qtrue : Qfalse;
}

static VALUE Parser_open_file(int argc, VALUE* argv, VALUE self) {
  VALUE filename;
  VALUE offset;
  VALUE length;
  rb_scan_args(argc, argv, "12", &filename, &offset, &length);

  Check_Type(filename, T_STRING);

  int64_t data_offset = 0;
  if (!NIL_P(offset)) {
    Check_Type(offset, T_FIXNUM);
    data_offset = NUM2INT(offset);
  }

  int64_t data_length = 0;
  if (!NIL_P(length)) {
    Check_Type(length, T_FIXNUM);
    data_length = NUM2INT(length);
  }

  auto parser = Parser_instance(self);
  auto success = parser_open_file_mmap(parser, str_from_rbstr(filename), data_offset, data_length);
  return success ? Qtrue : Qfalse;
}

static VALUE Parser_node(VALUE self) {
  auto parser = Parser_instance(self);
  auto node_ptr = raw_allocate_type(Node);
  *node_ptr = parser->node;
  return TypedData_Wrap_Struct(ruxmlNode, &Node_data_type, node_ptr);
}

static VALUE Parser_next_node(VALUE self) {
  auto parser = Parser_instance(self);
  get_node(parser);
  return parser->done ? Qfalse : Qtrue;
}

static VALUE Parser_done(VALUE self) {
  auto parser = Parser_instance(self);
  return parser->done ? Qtrue : Qfalse;
}

static VALUE Parser_errored(VALUE self) {
  auto parser = Parser_instance(self);
  return parser->errored ? Qtrue : Qfalse;
}

static VALUE Parser_node_column_start(VALUE self) {
  auto parser = Parser_instance(self);
  return INT2NUM(parser->node.c0);
}

static VALUE Parser_node_line(VALUE self) {
  auto parser = Parser_instance(self);
  return INT2NUM(parser->node.line);
}

static VALUE Parser_node_offset(VALUE self) {
  auto parser = Parser_instance(self);
  return INT2NUM(parser->node.offset);
}

static VALUE Parser_node_text(VALUE self) {
  auto parser = Parser_instance(self);
  return rbstr_from_str(parser->node.text);
}

static VALUE Parser_node_type(VALUE self) {
  auto parser = Parser_instance(self);
  return ID2SYM(node_type_ids[parser->node.type]);
}

static VALUE Parser_node_self_closing(VALUE self) {
  auto parser = Parser_instance(self);
  return parser->node.self_closing ? Qtrue : Qfalse;
}

//
// Init
//

void Init_ruxml() {
  node_type_ids[NODE_INVALID] = rb_intern("invalid");
  node_type_ids[NODE_ELEMENT_BEGIN] = rb_intern("begin");
  node_type_ids[NODE_ELEMENT_END] = rb_intern("end");
  node_type_ids[NODE_TEXT] = rb_intern("text");
  node_type_ids[NODE_XML_HEADER] = rb_intern("xml_header");
  node_type_ids[NODE_COMMENT] = rb_intern("comment");

  ruxmlModule = rb_define_module("RUXML");

  ruxmlNode = rb_define_class_under(ruxmlModule, "Node", rb_cData);
  rb_define_alloc_func(ruxmlNode, Node_allocate);
  rb_define_method(ruxmlNode, "initialize", reinterpret_cast<VALUE (*)(...)>(Node_initialize), 0);
  rb_define_method(ruxmlNode, "column_start", reinterpret_cast<VALUE (*)(...)>(Node_column_start), 0);
  rb_define_method(ruxmlNode, "line", reinterpret_cast<VALUE (*)(...)>(Node_line), 0);
  rb_define_method(ruxmlNode, "offset", reinterpret_cast<VALUE (*)(...)>(Node_offset), 0);
  rb_define_method(ruxmlNode, "text", reinterpret_cast<VALUE (*)(...)>(Node_text), 0);
  rb_define_method(ruxmlNode, "type", reinterpret_cast<VALUE (*)(...)>(Node_type), 0);
  rb_define_method(ruxmlNode, "self_closing", reinterpret_cast<VALUE (*)(...)>(Node_self_closing), 0);

  ruxmlParser = rb_define_class_under(ruxmlModule, "Parser", rb_cData);
  rb_define_alloc_func(ruxmlParser, Parser_allocate);
  rb_define_method(ruxmlParser, "initialize", reinterpret_cast<VALUE (*)(...)>(Parser_initialize), 0);
  rb_define_method(ruxmlParser, "open_string", reinterpret_cast<VALUE (*)(...)>(Parser_open_string), -1);
  rb_define_method(ruxmlParser, "open_file", reinterpret_cast<VALUE (*)(...)>(Parser_open_file), -1);
  rb_define_method(ruxmlParser, "node", reinterpret_cast<VALUE (*)(...)>(Parser_node), 0);
  rb_define_method(ruxmlParser, "next_node", reinterpret_cast<VALUE (*)(...)>(Parser_next_node), 0);
  rb_define_method(ruxmlParser, "done", reinterpret_cast<VALUE (*)(...)>(Parser_done), 0);
  rb_define_method(ruxmlParser, "errored", reinterpret_cast<VALUE (*)(...)>(Parser_errored), 0);

  rb_define_method(ruxmlParser, "node_column_start", reinterpret_cast<VALUE (*)(...)>(Parser_node_column_start), 0);
  rb_define_method(ruxmlParser, "node_line", reinterpret_cast<VALUE (*)(...)>(Parser_node_line), 0);
  rb_define_method(ruxmlParser, "node_offset", reinterpret_cast<VALUE (*)(...)>(Parser_node_offset), 0);
  rb_define_method(ruxmlParser, "node_text", reinterpret_cast<VALUE (*)(...)>(Parser_node_text), 0);
  rb_define_method(ruxmlParser, "node_type", reinterpret_cast<VALUE (*)(...)>(Parser_node_type), 0);
  rb_define_method(ruxmlParser, "node_self_closing", reinterpret_cast<VALUE (*)(...)>(Parser_node_self_closing), 0);
}

}