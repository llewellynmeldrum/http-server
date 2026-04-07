#pragma once
// ByteStream.h
#include "CWrappers.h"
#include "Logger.h"
#include "StringView.h"
#include <ctype.h>
#include <stddef.h>
#include <stdio.h>
typedef struct {
    Byte*  data;
    size_t size;
    size_t next;
} ByteStream;

static inline char bs_peek1(const ByteStream* bs) {
    return bs->next < bs->size ? bs->data[bs->next] : '\0';
}
static inline char bs_peek(const ByteStream* bs, size_t n) {
    return bs->next + n < bs->size ? bs->data[bs->next + n] : '\0';
}
static inline char bs_peek2(const ByteStream* bs) {
    return bs->next + 1 < bs->size ? bs->data[bs->next + 1] : '\0';
}
static inline char bs_consume(ByteStream* bs) {
    return bs->next < bs->size ? bs->data[bs->next++] : '\0';
}
static inline char bs_peekPrevious(const ByteStream* bs) {
    return bs->next - 1 >= 0 ? bs->data[bs->next - 1] : '\0';
}

static inline Byte* bs_getCurrent(const ByteStream* bs) {
    return bs->next < bs->size ? &bs->data[bs->next] : nullptr;
}

static inline size_t bs_consumeWhitespace(ByteStream* bs) {
    size_t n_whitespace = 0;

    while (isspace(bs_peek1(bs))) {
        bs_consume(bs);
    }
    return n_whitespace;
}
static inline StringView bs_consumeUntilAnyDelim(ByteStream* bs, StringView delims) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    // loop until we find a delimiter
    while (bs_peek1(bs) && !sv_contains(delims, bs_peek1(bs))) {
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr, "Word is longer than BUF_SZ (%zu), result may be truncated.", BUF_SZ);
    }
    res.len = sv_len;
    return res;
}
static inline StringView bs_consumeUntil(ByteStream* bs, StringView target_sv) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    char   ch;
    size_t target_len = target_sv.len;
    while ((ch = bs_peek1(bs))) {
        if (ch == sv_at(target_sv, 0)) {
            int i = 1;
            while (i < target_len) {
                char lookahead_ch = bs_peek(bs, i);
                if (lookahead_ch && lookahead_ch == sv_at(target_sv, i)) {
                    i++;
                } else {
                    break;
                }
            }
            if (i == target_len) {
                break;  // found whole string
            }
        }
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr, "Word is longer than BUF_SZ (%zu), result may be truncated.", BUF_SZ);
    }
    res.len = sv_len;
    return res;
}

static inline StringView bs_lookahead(ByteStream* bs, size_t n) {
    // BUG: Doesnt account for if bs has <n bytes remaning
    StringView s = {
        .ptr = bs_getCurrent(bs),
        .len = n,
    };
    return s;
}
// **NON-CONSUMING**
// - returns `true` *IFF* the next `n` chars of `bs` match `target_sv` exactly.
// - (where `n = target_sv.len`)
static inline bool bs_lookaheadMatches(ByteStream* bs, StringView target_sv) {
    for (size_t i = 0; i < target_sv.len; i++) {
        if (!bs_peek(bs, i)) {
            return false;
        }
        if (bs_peek(bs, i) != target_sv.ptr[i]) {
            return false;
        }
    }
    return true;
}
static inline bool bs_skipExactly(ByteStream* bs, StringView exact_sv) {
    size_t i = 0;
    char   ch;
    while ((ch = bs_peek1(bs))) {
        if (ch != sv_at(exact_sv, i)) {
            break;
        }
        i++;
        bs_consume(bs);
    }
    return i == exact_sv.len;
}

static inline void bs_debugLog(ByteStream* bs) {
    print_buffer_verbose("ByteStream", bs->data, bs->size, 0);
}
static inline StringView bs_consumeUntilChar(ByteStream* bs, const char delim) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    while (bs_peek1(bs) && bs_peek1(bs) != delim) {
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr, "Word is longer than BUF_SZ (%zu), result may be truncated.", BUF_SZ);
    }
    res.len = sv_len;
    return res;
}

static inline StringView bs_consumeRemaining(ByteStream* bs) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t count = 0;
    while (bs_peek(bs, 1)) {
        bs_consume(bs);
        count++;
    }
    res.len = count;
    return res;
}
static inline StringView bs_consumeN(ByteStream* bs, size_t n) {
    if (n >= BUF_SZ) {
        fprintf(stderr, "Word is longer than BUF_SZ (%zu), result may be truncated.", BUF_SZ);
    }
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t slice_len = 0;
    while (bs_peek1(bs) && slice_len < n) {
        slice_len++;
        bs_consume(bs);
    }
    res.len = slice_len;
    return res;
}
static inline StringView bs_consumeWord(ByteStream* bs) {
    StringView res = {};
    res.ptr = bs_getCurrent(bs);
    size_t sv_len = 0;
    while (bs_peek1(bs) && isalphanumeric(bs_peek1(bs))) {
        sv_len++;
        bs_consume(bs);
    }

    if (sv_len >= BUF_SZ) {
        fprintf(stderr, "Word is longer than BUF_SZ (%zu), result may be truncated.", BUF_SZ);
    }
    res.len = sv_len;
    //    LOG_DEBUG("(%d)%.*s", res.len, res.len, res.ptr);
    return res;
}

static inline ByteStream bs_make(void* data, size_t size) {
    return (ByteStream){
        .data = data,
        .size = size,
        .next = 0,
    };
}
