#pragma once
// ByteStream.h
#include "StringView.h"
#include "myutils.h"
#include "types.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
typedef struct {
    Byte *data;
    size_t size;
    size_t next;
} ByteStream;

static inline char bs_peek(const ByteStream *bs) {
    return bs->next < bs->size ? bs->data[bs->next] : '\0';
}
static inline char bs_consume(ByteStream *bs) {
    return bs->next < bs->size ? bs->data[bs->next++] : '\0';
}
static inline char bs_peekPrevious(const ByteStream *bs) {
    return bs->next - 1 >= 0 ? bs->data[bs->next - 1] : '\0';
}

static inline Byte *bs_getCurrent(const ByteStream *bs) {
    return bs->next < bs->size ? &bs->data[bs->next] : nullptr;
}

static inline size_t bs_consumeWhitespace(ByteStream *bs) {
    size_t n_whitespace = 0;

    while (isspace(bs_peek(bs))) {
        bs_consume(bs);
    }
    return n_whitespace;
}
static inline StringView bs_consumeUntilAnyDelim(ByteStream *bs,
                                                 StringView delims) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    // loop until we find a delimiter
    while (bs_peek(bs) && !sv_contains(delims, bs_peek(bs))) {
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr,
                "Word is longer than BUF_SZ (%zu), result may be truncated.",
                BUF_SZ);
    }
    res.len = sv_len;
    return res;
}

static inline StringView bs_consumeUntilDelim(ByteStream *bs,
                                              const char delim) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    while (bs_peek(bs) && bs_peek(bs) != delim) {
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr,
                "Word is longer than BUF_SZ (%zu), result may be truncated.",
                BUF_SZ);
    }
    res.len = sv_len;
    return res;
}

static inline StringView bs_consumeN(ByteStream *bs, size_t n) {
    if (n >= BUF_SZ) {
        fprintf(stderr,
                "Word is longer than BUF_SZ (%zu), result may be truncated.",
                BUF_SZ);
    }
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t slice_len = 0;
    while (bs_peek(bs)) {
        slice_len++;
        bs_consume(bs);
    }
    res.len = slice_len;
    return res;
}
static inline StringView bs_consumeWord(ByteStream *bs) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    while (bs_peek(bs) && isalphanumeric(bs_peek(bs))) {
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr,
                "Word is longer than BUF_SZ (%zu), result may be truncated.",
                BUF_SZ);
    }
    res.len = sv_len;
    return res;
}

static inline ByteStream bs_make(void *data, size_t size) {
    return (ByteStream){
        .data = data,
        .size = size,
        .next = 0,
    };
}
