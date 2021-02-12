#pragma once

#include <cstdint>
#include <cstring>

#include "memory.hpp"

size_t zstr_length(const char *str);
char *zstr_dup(const char *str);
char *zstr_dup(const char *str, int64_t size);
int zstr_find_last(const char *str, char c);

struct String {
   int32_t length;
   char *data;
};

inline String operator "" _str(const char *data, size_t length) {
   return String{static_cast<int32_t>(length),
                 const_cast<char *>(data)};
}

inline String copy_string(MemoryArena *arena, uint32_t length, char *input) {
   uint32_t buf_length = length + 1;

   String str;
   str.length = length;
   str.data = (char *) allocate_size(arena, buf_length);
   memcpy(str.data, input, length);
   str.data[length] = 0;
   return str;
}

inline String copy_string(MemoryArena *arena, String input) {
   return copy_string(arena, input.length, input.data);
}

inline String copy_zstring(MemoryArena *arena, char *input) {
   return copy_string(arena, strlen(input), input);
}

inline String as_zstring(char *input) {
   String str;
   str.length = static_cast<int32_t>(zstr_length(input));
   str.data = input;
   return str;
}

inline String str_from_index(String s, int index) {
   return String{s.length - index, s.data + index};
}

inline String str_until_index(String s, int index) {
   return String{index + 1, s.data};
}

inline String str_after_index(String s, int index) {
   return String{s.length - index - 1, s.data + index + 1};
}

inline String str_before_index(String s, int index) {
   return String{index, s.data};
}

inline String str_between(String s, int after_index, int before_index) {
   return String{before_index - after_index - 1, s.data + after_index + 1};
}

inline String str_empty() {
   return String{0, nullptr};
}

bool parse_int(String string, int32_t *result_ptr);

inline int32_t parse_int_default(String string, int32_t default_value) {
   int32_t result;
   if (!parse_int(string, &result)) result = default_value;
   return result;
}

bool parse_int64(String string, int64_t *result_ptr);

inline int64_t parse_int64_default(String string, int64_t default_value) {
   int64_t result;
   if (!parse_int64(string, &result)) result = default_value;
   return result;
}

// Use in printf with format specifier  %.*s
#define str_prt(s) s.length, s.data

String str_dup(String str);
String str_dup(const char *str);
String str_dup(const char *str, int length);
String str_dup(Allocator *allocator, String str);
String str_dup(Allocator *allocator, const char *str);
String str_dup(Allocator *allocator, const char *str, int length);

char* str_to_zstr(String str);
char* str_to_zstr(Allocator *allocator, String str);

String str_print(const char *fmt, ...);
String str_print(Allocator *allocator, const char *fmt, ...);

int str_find_last(String s, char c, int after = 0);
int str_find_first(String s, char c, int after = 0);
int str_compare(String a, String b);
bool str_equal(String a, String b);
bool str_equal(String a, const char *b);
bool str_equal(String a, const char *b_data, int b_length);
bool str_empty(String s);