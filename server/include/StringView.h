#pragma once

#include "http_consts.h"
#include "types.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Byte *ptr;
    size_t len;
} StringView;
typedef struct {
    StringView l;
    StringView r;
} StringViewPair;

static inline StringViewPair svp_make(StringView l, StringView r) {
    return (StringViewPair){
        .l = l,
        .r = r,
    };
}
static inline StringView sv_make(char *cstr) {
    return (StringView){
        .ptr = (Byte *)cstr,
        .len = cstr ? strlen(cstr) : 0,
    };
}

static inline bool sv_matchesStr(const StringView sv, const char *str) {
    if (sv.len != strnlen(str, BUF_SZ)) {
        return false;
    }
    for (int i = 0; i < sv.len; i++) {
        if (str[i] != sv.ptr[i]) {
            return false;
        }
    }
    return true;
}

static inline const char *sv_toStr(const StringView sv) {
    char *cstr = calloc(sv.len + 1, 1);
    snprintf(cstr, sv.len, "'%s'", sv.ptr);
    // BUG: leak, only for debugging so should be fine
    return cstr;
}

static inline const char *svp_toStr(const StringViewPair svp) {
    size_t len = svp.l.len + 4 + svp.r.len;
    char *cstr = calloc(len, 1);
    snprintf(cstr, len, "%s,%s", sv_toStr(svp.l), sv_toStr(svp.r));
    return cstr;
}

static inline const char *svparr_ToStr(const StringViewPair *svparr,
                                       size_t sz) {
    size_t total_len = {};
    for (size_t i = 0; i < sz; i++) {
        total_len += svparr[i].l.len + 7 + svparr[i].r.len;
        // {[<>,<>],\n}
    }
    char *cstr = calloc(total_len, 1);
    snprintf(cstr, total_len, "%s,\n", svp_toStr(svparr[0]));
    for (size_t i = 1; i < sz; i++) {
        snprintf(cstr, total_len, "%s,\n%s", cstr, svp_toStr(svparr[i]));
    }
    return cstr;
}
static inline bool sv_isEmpty(const StringView sv) { return sv.len == 0; }
// linear search
static inline bool sv_hasPrefixStr(const StringView sv, const char *s) {
    for (int i = 0; i < strlen(s); i++) {
        if (sv.ptr[i] != s[i])
            return false;
    }
    return true;
}
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
