#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

typedef uint64_t u64;
typedef uint32_t u32;
typedef uint16_t u16;
typedef uint8_t  u8;

typedef int64_t i64;
typedef int32_t i32;
typedef int16_t i16;
typedef int8_t  i8;

typedef char Byte;

#define arrlen(x) (sizeof(x) / sizeof(x[0]))

static inline int RAND_RANGE(int min, int max) {
    return (min + (rand() % (max - min)));
}

static inline bool streq(const char* s1, const char* s2) {
    return !strcmp(s1, s2);
}
static inline bool isalphanumeric(const char ch) {
    return isalpha(ch) || isdigit(ch);
}
