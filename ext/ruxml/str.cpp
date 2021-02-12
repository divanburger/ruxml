//
// Created by divan on 27/12/18.
//

#include <cstdarg>
#include <cstdio>
#include "str.hpp"

uint64_t zstr_length(const char *str) {
  uint64_t result = 0;
  while (*(str++)) result++;
  return result;
}

char *zstr_dup(const char *str, int64_t size) {
  char *result = raw_allocate_string_zt(size);
  memcpy(result, str, sizeof(char) * size);
  return result;
}

char *zstr_dup(const char *str) {
  return zstr_dup(str, zstr_length(str));
}

char *str_to_zstr(Allocator *allocator, String s) {
  char *result = (char *) allocate_size(allocator, (size_t) (s.length + 1));
  memcpy(result, s.data, sizeof(char) * s.length);
  result[s.length] = 0;
  return result;
}

char *str_to_zstr(String s) { return str_to_zstr(temp_allocator, s); }

String str_dup(Allocator *allocator, String s) {
  String result;
  result.length = s.length;
  result.data = (char *) allocate_size(allocator, (size_t) (s.length + 1));
  memcpy(result.data, s.data, sizeof(char) * s.length);
  result.data[s.length] = 0;
  return result;
}

String str_dup(String s) { return str_dup(temp_allocator, s); }

String str_dup(Allocator *allocator, const char *str) {
  return str_dup(allocator, str, static_cast<int>(zstr_length(str)));
}

String str_dup(const char *str) { return str_dup(temp_allocator, str); }

String str_dup(Allocator *allocator, const char *str, int length) {
  String result;
  result.length = length;
  result.data = allocate_zstring(allocator, length);
  memcpy(result.data, str, sizeof(char) * length);
  result.data[length] = 0;
  return result;
}

String str_dup(const char *str, int length) { return str_dup(temp_allocator, str, length); }

int zstr_find_last(const char *str, char c) {
  int result = -1;
  int index = 0;
  while (*str != 0) {
    if (*str == c) result = index;
    str++;
    index++;
  }
  return result;
}

int str_find_last(String s, char c, int after) {
  int result = -1;
  int index = 0;
  char *str = s.data + after;
  for (int i = after; i < s.length; i++) {
    if (*str == c) result = index;
    str++;
    index++;
  }
  return result;
}

int str_find_first(String s, char c, int after) {
  char *str = s.data + after;
  for (int i = after; i < s.length; i++) {
    if (*str == c) return i;
    str++;
  }
  return -1;
}

bool str_equal(String a, String b) {
  if (a.length != b.length) return false;
  return memcmp(a.data, b.data, a.length) == 0;
}

bool str_equal(String a, const char *b) {
  return str_equal(a, b, zstr_length(b));
}

bool str_equal(String a, const char *b_data, int b_length) {
  if (a.length != b_length) return false;
  return memcmp(a.data, b_data, a.length) == 0;
}

int str_compare(String a, String b) {
  if (a.length < b.length) return -1;
  if (b.length > a.length) return 1;
  return memcmp(a.data, b.data, a.length);
}

bool str_empty(String s) {
  return s.length == 0 || !s.data;
}

bool parse_int(String string, int32_t *result_ptr) {
  bool valid = false;
  int result = 0;

  auto buffer = string.data;

  for (int i = 0; i < string.length; i++) {
    char c = buffer[0];
    if (c >= '0' && c <= '9') {
      result *= 10;
      result += c - '0';
      valid = true;
    } else {
      valid = false;
      break;
    }
    buffer++;
  }

  *result_ptr = result;
  return valid;
}

bool parse_int64(String string, int64_t *result_ptr) {
  bool valid = false;
  int result = 0;

  auto buffer = string.data;

  for (int i = 0; i < string.length; i++) {
    char c = buffer[0];
    if (c >= '0' && c <= '9') {
      result *= 10;
      result += c - '0';
      valid = true;
    } else {
      valid = false;
      break;
    }
    buffer++;
  }

  *result_ptr = result;
  return valid;
}

String str_print(Allocator *allocator, const char *fmt, ...) {
  char buffer[1024];

  va_list v, v2;
  va_start(v, fmt);
  va_copy(v2, v);

  auto res = vsnprintf(buffer, sizeof(buffer), fmt, v);
  va_end(v);

  if (res <= array_size(buffer)) {
    return str_dup(allocator, buffer, res);
  } else {
    int big_size = res + 1;

    String result;
    result.length = res;
    result.data = allocate_zstring(allocator, res);
    res = vsnprintf(result.data, static_cast<size_t>(big_size), fmt, v2);

    va_end(v2);
    assert(res >= 0);

    return result;
  }
}

String str_print(const char *fmt, ...) {
  char buffer[1024];

  va_list v, v2;
  va_start(v, fmt);
  va_copy(v2, v);

  auto res = vsnprintf(buffer, sizeof(buffer), fmt, v);
  va_end(v);

  if (res <= array_size(buffer)) {
    return str_dup(temp_allocator, buffer, res);
  } else {
    int big_size = res + 1;

    String result;
    result.length = res;
    result.data = allocate_zstring(temp_allocator, res);
    res = vsnprintf(result.data, static_cast<size_t>(big_size), fmt, v2);

    va_end(v2);
    assert(res >= 0);

    return result;
  }
}