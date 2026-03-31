#pragma once

#include "http_consts.h"
#include "logger.h"
#include "types.h"
#include <ctype.h>
#include <string.h>
typedef struct {
    Byte*  ptr;
    size_t len;
} StringView;

static inline StringView sv_make(char* cstr) {
    return (StringView){
        .ptr = (Byte*)cstr,
        .len = cstr ? strlen(cstr) : 0,
    };
}

static inline void sv_print(const StringView sv) {
    Byte buf[BUF_SZ] = {};
    memcpy(buf, sv.ptr, sv.len);
    LOG_DEBUG("sv.len=%zu, sv.cstr='%s'", sv.len, sv.ptr);
}

static inline bool sv_isEmpty(const StringView sv) {
    return sv.len == 0;
}
// linear search
static inline bool sv_contains(const StringView sv, const char s) {
    for (int i = 0; i < sv.len; i++) {
        if (sv.ptr[i] == s)
            return true;
    }
    return false;
}
static inline void sv_toLower(StringView sv, const char s) {
    for (int i = 0; i < sv.len; i++) {
        if (isupper(sv.ptr[i])) {
            sv.ptr[i] = tolower(sv.ptr[i]);
        }
    }
}
static inline void sv_toUpper(StringView sv, const char s) {
    for (int i = 0; i < sv.len; i++) {
        if (islower(sv.ptr[i])) {
            sv.ptr[i] = toupper(sv.ptr[i]);
        }
    }
}
