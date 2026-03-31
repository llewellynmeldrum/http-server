#pragma once
#include <ctype.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#define IS_SAME_TYPE(T1, T2) __builtin_types_compatible_p(T1, T2)

// is the array the type as a pointer to an element of it
#define IS_ARRAY(arr) (!IS_SAME_TYPE(typeof(arr), typeof(&arr[0])))

// index into -1 if !IS_ARRAY, compiler error for arrlen(ptr_type)
#define arrlen(x)                                                                                  \
    ((sizeof(x) / sizeof(x)) /* check if array_type */                                             \
     + (0 * x[(IS_ARRAY(x) ? 1 : -1)]))

static inline int RAND_RANGE(int min, int max) {
    return (min + (rand() % (max - min)));
}

static inline bool streq(const char* s1, const char* s2) {
    return !strcmp(s1, s2);
}
static inline bool isalphanumeric(const char ch) {
    return isalpha(ch) && isdigit(ch);
}
