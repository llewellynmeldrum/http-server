#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

// index into -1 if !IS_ARRAY, compiler error for arrlen(ptr_type)
#define arrlen(x) (sizeof(x) / sizeof(x[0])) /* check if array_type */

static inline int RAND_RANGE(int min, int max) {
    return (min + (rand() % (max - min)));
}

static inline bool streq(const char *s1, const char *s2) {
    return !strcmp(s1, s2);
}
static inline bool isalphanumeric(const char ch) {
    return isalpha(ch) && isdigit(ch);
}
