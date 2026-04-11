
#include "Types.h"
#include <stddef.h>
#include <stdlib.h>
struct ByteArray {
    Byte*  ptr;
    size_t len;
    size_t cap;
};
typedef struct ByteArray ByteArray;

static inline ByteArray ba_make(size_t capacity) {
    return (ByteArray){
        .ptr = calloc(capacity, sizeof(Byte)),
        .len = 0,
        .cap = capacity,
    };
}
static inline size_t ba_remaining(ByteArray* ba) {
    return ba->cap - ba->len;
}
static inline size_t ba_advance(ByteArray* ba, size_t count) {
    return ba->len += count;
}
static inline Byte* ba_front(const ByteArray* ba) {
    return ba->len < ba->cap ? ba->ptr + ba->len : nullptr;
}
