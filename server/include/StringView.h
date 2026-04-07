#pragma once
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "DebugPrinting.h"
#include "Globals.h"
#include "Types.h"

struct StringView {
    Byte*  ptr;
    size_t len;
    bool   alloced;
};
// **NON OWNING**, length-based string class.
typedef struct StringView StringView;

struct StringViewPair {
    StringView l;
    StringView r;
};

static const StringView NULL_STRINGVIEW = {
    .len = 0,
    .ptr = nullptr,
};

typedef struct StringViewPair StringViewPair;

static const StringView CR = { "\r", 1 };
static const StringView LF = { "\n", 1 };
static const StringView SP = { " ", 1 };
static const StringView HTAB = { "\t", 1 };
static const StringView CRLF = { "\r\n", 2 };
static const StringView DOT = { ".", 1 };
static const StringView DOTDOT = { "..", 2 };
static const StringView EMPTY = { "", 0 };
static const StringView FSLASH = { "/", 1 };
static const StringView INDEX_HTML = { "index.html", 10 };
static const StringView DOCROOT = { "../www", 6 };
#ifdef __APPLE__
static const StringView RESOLVED_DOCROOT = { "/Users/llewie/code/proj/arch-http/www", 37 };
#else
#error "havent set this up"
static const StringView RESOLVED_DOCROOT = { "", 37 };

#endif

// **HTAB|SP**
static const StringView OWS = { " \t", 2, false };

static const StringView COLON = { ":", 1, false };

static inline char* sv_cstr(const StringView sv) {
    if (sv.len == 0) {
        return "";
    }
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
static inline char sv_at(const StringView sv, const size_t i) {
    return i < sv.len ? sv.ptr[i] : '\0';
}
// should be inclusive to `new_start`
[[nodiscard("This function performs in place modification")]]
static inline StringView sv_trimLeft(StringView sv, size_t new_start) {
    //
    if (sv.len - new_start < 0) {
        // BUG: trim makes string negative length.
        fprintf(stderr, "Warning! trimLeft of sv resulted in negative length.");
        sv.len = 0;
        return sv;
    }
    sv.ptr += new_start;  // 5:'abcde' -> 3 -> 2:'de'
    sv.len -= new_start;
    return sv;
}
// `new_end` is exclusive, i.e the resulting object does not include new_end
[[nodiscard("This function performs in place modification")]]
static inline StringView sv_trimRight(StringView sv, size_t new_end) {
    //
    if (sv.len - new_end < 0) {
        // BUG: trim makes string negative length.
        fprintf(stderr, "Warning! trimLeft of sv resulted in negative length.");
        sv.len = 0;
        return sv;
    }
    // eg:
    // trimRight('helloworld',5) = 'hello'
    // 'helloworld'
    //  0123456789
    sv.len = new_end;
    return sv;
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
static inline bool sv_equal(const StringView self, const StringView other) {
    if (self.len != other.len) {
        return false;
    }
    for (int i = 0; i < self.len; i++) {
        if (self.ptr[i] != other.ptr[i]) {
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
static inline StringView sv_copy(const StringView self, Byte* copybuf) {
    memcpy(copybuf, self.ptr, self.len);
    StringView copy = (StringView){
        .ptr = copybuf,
        .len = self.len,
    };
    return copy;
}
static inline size_t sv_find(const StringView self, const char ch) {
    for (int i = 0; i < self.len; i++) {
        if (self.ptr[i] == ch)
            return i;
    }
    return self.len;
}
static inline StringViewPair sv_split(StringView self, size_t delim_offset) {
    // 7:'defg' -> 2 -> 4:'defg'
    StringView lhs = self;
    StringView rhs = self;

    lhs.len = delim_offset;

    rhs.ptr += delim_offset + 1;
    rhs.len = self.len - (delim_offset + 1);

    return (StringViewPair){
        .l = lhs,
        .r = rhs,
    };
}
static inline StringViewPair sv_splitOn(StringView self, char delim) {
    return sv_split(self, sv_find(self, delim));
}
static inline bool sv_contains(const StringView self, const char ch) {
    //    printf("(%zu)'%s' does not contain '%c'\n", sv.len, sv_toStr(sv), ch);
    for (int i = 0; i < self.len; i++) {
        if (self.ptr[i] == ch)
            return true;
    }
    return false;
}
// Every instance of a character in `self` which matches one from  `target` is
// deleted. Everything is shifted to the left in the instance of a deletion.
static inline StringView sv_strip(StringView self, const StringView targets) {
    Byte             copybuf[BUF_SZ] = {};
    const StringView original = sv_copy(self, copybuf);

    size_t writehead = 0;
    size_t readhead = 0;

    while (readhead < original.len) {
        if (!sv_contains(targets, original.ptr[readhead])) {
            // if we encounter a non target char, write it
            self.ptr[writehead++] = original.ptr[readhead++];
        } else {
            // otherwise, advance readhead
            readhead++;
        }
    }
    self.len = writehead;  // however many characters we kept
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
    char* cstr = sv_cstr(sv);
    printf("%s->[%zu]:'%s'\n", prefix, sv.len, cstr);
    free(cstr);
    // BUG: leak, only for debugging so should be fine
    return cstr;
}

static inline const char* svp_toStr(const StringViewPair svp) {
    char* cstr = calloc(BUF_SZ, 1);
    snprintf(cstr, BUF_SZ, "(%s)=(%s)", sv_cstr(svp.l), sv_cstr(svp.r));
    return cstr;
}

static inline const char* svparr_ToStr(const StringViewPair* svparr, size_t sz) {
    size_t total_len = {};
    for (size_t i = 0; i < sz; i++) {
        total_len += svparr[i].l.len + 7 + svparr[i].r.len;
        // {[<>,<>],\n}
    }
    char* cstr = calloc(total_len, 1);
    strncat(cstr, "{\n", total_len);
    for (size_t i = 0; i < sz; i++) {
        char buf[BUF_SZ] = {};
        snprintf(buf, total_len, "\t\t[%zu]:%s\n", i, svp_toStr(svparr[i]));
        strncat(cstr, buf, total_len);
    }
    strncat(cstr, "\t}", total_len);
    return cstr;
}
