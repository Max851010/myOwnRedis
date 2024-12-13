#pragma once
#include <stddef.h>
#include <stdint.h>

#define container_of(ptr, type, member)                                        \
  reinterpret_cast<type *>(reinterpret_cast<char *>(ptr) -                     \
                           offsetof(type, member))

enum {
  SER_NIL = 0, // Like `NULL`
  SER_ERR = 1, // An error code and message
  SER_STR = 2, // A string
  SER_INT = 3, // A int64
  SER_DBL = 4,
  SER_ARR = 5, // Array
};

// FNV-1a Algorithm
static uint64_t strHash(const uint8_t *data, size_t length) {
  uint32_t h = 0x811C9DC5;
  for (size_t i = 0; i < length; i++) {
    h = (h + data[i]) * 0x01000193;
  }
  return h;
}
