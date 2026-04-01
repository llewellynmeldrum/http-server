#pragma once

#include "CWrappers.h"
#include "DebugPrinting.h"
#include "Globals.h"
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    Byte*  ptr;
    size_t len;
    bool   alloced;
} StringView;
typedef struct {
    StringView l;
    StringView r;
} StringViewPair;

static inline char* sv_toStr(const StringView sv) {
    char* cstr = calloc(sv.len + 1, 1);
    snprintf(cstr, sv.len + 1, "%s", sv.ptr);
    //    print_buffer_verbose("StringView", sv.ptr, sv.len, 0);
    // BUG: leak, only for debugging so should be fine
    return cstr;
}
static inline StringViewPair svp_make(StringView l, StringView r) {
    return (StringViewPair){
        .l = l,
        .r = r,
    };
}
static inline StringView sv_make(char* cstr) {
    return (StringView){
        .ptr = (Byte*)cstr,
        .len = cstr ? strlen(cstr) : 0,
        .alloced = false,
    };
}

static inline bool sv_matchesStr(const StringView sv, const char* str) {
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

static inline bool sv_isEmpty(const StringView sv) {
    return sv.len == 0;
}
// linear search
static inline bool sv_hasPrefixStr(const StringView sv, const char* s) {
    for (int i = 0; i < strlen(s); i++) {
        if (sv.ptr[i] != s[i])
            return false;
    }
    return true;
}
static inline StringView sv_allocCopy(const StringView sv) {
    StringView copy = (StringView){
        .ptr = calloc(sv.len, 1),
        .len = sv.len,
        .alloced = true,
    };
    memcpy(copy.ptr, sv.ptr, sv.len);
    return copy;
}
static inline bool sv_free(const StringView sv) {
    if (sv.alloced) {
        memset(sv.ptr, 0x0, sv.len);
        free(sv.ptr);
        return true;
    } else {
        fprintf(stderr, "Attempted to free StringView which was not marked as allocated: %p",
                sv.ptr);
        exit(EXIT_FAILURE);
        return false;
    }
}

static inline bool sv_contains(const StringView sv, const char ch) {
    //    printf("(%zu)'%s' does not contain '%c'\n", sv.len, sv_toStr(sv), ch);
    for (int i = 0; i < sv.len; i++) {
        if (sv.ptr[i] == ch)
            return true;
    }
    return false;
}
// Every instance of a character in `self` which matches one from  `target` is deleted.
// Everything is shifted to the left in the instance of a deletion.
//
static inline StringView sv_strip(StringView self, const StringView targets) {
    // TODO:: remove the heap alloc here
    const StringView copy = sv_allocCopy(self);

    size_t writehead = 0;
    size_t readhead = 0;

    while (readhead < copy.len) {
        if (!sv_contains(targets, copy.ptr[readhead])) {
            // if we encounter a non target char, write it
            //            printf("nostrip:%c\n", copy.ptr[readhead]);
            self.ptr[writehead++] = copy.ptr[readhead++];
        } else {
            //            printf("strip:%c\n", copy.ptr[readhead]);
            // otherwise, advance readhead
            readhead++;
        }
    }
    sv_free(copy);
    self.len = writehead;
    return self;
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
// bullshit:
#define sv_print(sv) _SV_PRINT(#sv, sv)
static inline const char* _SV_PRINT(const char* prefix, const StringView sv) {
    char* cstr = sv_toStr(sv);
    printf("%s->[%zu]:'%s'\n", prefix, sv.len, cstr);
    free(cstr);
    // BUG: leak, only for debugging so should be fine
    return cstr;
}

static inline const char* svp_toStr(const StringViewPair svp) {
    char* cstr = calloc(BUF_SZ, 1);
    snprintf(cstr, BUF_SZ, "%s,%s", sv_toStr(svp.l), sv_toStr(svp.r));
    return cstr;
}

static inline const char* svparr_ToStr(const StringViewPair* svparr, size_t sz) {
    size_t total_len = {};
    for (size_t i = 0; i < sz; i++) {
        total_len += svparr[i].l.len + 7 + svparr[i].r.len;
        // {[<>,<>],\n}
    }
    char* cstr = calloc(total_len, 1);
    snprintf(cstr, total_len, "%s,\n", svp_toStr(svparr[0]));
    for (size_t i = 1; i < sz; i++) {
        snprintf(cstr, total_len, "%s,\n%s", cstr, svp_toStr(svparr[i]));
    }
    return cstr;
}
