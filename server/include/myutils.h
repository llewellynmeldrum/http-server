#pragma once
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
static inline int RAND_RANGE(int min, int max) {
    return (min + (rand() % (max - min)));
}

static inline bool streq(const char *s1, const char *s2) {
    return !strcmp(s1, s2);
}
