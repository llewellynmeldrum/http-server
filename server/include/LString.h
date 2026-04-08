#pragma once
#include <assert.h>

#include "Logger.h"
#include "StringView.h"

#include "CWrappers.h"
static constexpr size_t SSO_CAP = 25;
struct String {
    bool isShort;
    union {
        struct {
            char*  data;
            size_t cap;
        };
        char short_data[SSO_CAP];
    };
    size_t len;
    bool   isNull;
};

typedef struct String String;

static const String NULL_STRING = {
    .isNull = true,
};

static inline bool str_reserve(String* self, size_t new_len) {
    if (self->isShort && new_len > SSO_CAP) {
        char copybuf[SSO_CAP] = {};
        memcpy(copybuf, self->short_data, self->len);
        self->isShort = false;
        self->data = calloc(new_len, sizeof(char));
        memcpy(self->data, copybuf, self->len);
    }

    if (!(self->isShort) && new_len > SSO_CAP) {
        self->data = realloc(self->data, new_len);
        self->cap = new_len;
    }
    if (!(self->isShort) && new_len <= SSO_CAP) {
        char copybuf[SSO_CAP] = {};
        memcpy(copybuf, self->data, self->len);
        free(self->data);
        self->isShort = true;
        memcpy(self->short_data, copybuf, self->len);
        self->cap = new_len;
    }
    return true;  // TODO: implement error checks
}

static inline char* str_ptr(String* self) {
    return self->isShort ? self->short_data : self->data;
}
static inline char* str_end(String* self) {
    return str_ptr(self) + self->len;
}
static inline char str_at(const String* self, size_t n) {
    return self->isShort ? self->short_data[n] : self->data[n];
}
static inline char str_last(String* self) {
    return str_at(self, self->len - 1);
}
static inline void str_set(String* self, size_t n, const char ch) {
    *(str_ptr(self) + n) = ch;
}

static inline bool str_equal(const String* self, const String* other) {
    if (self->len != other->len)
        return false;
    for (size_t i = 0; i < self->len; i++) {
        if (str_at(self, i) != str_at(other, i)) {
            return false;
        }
    }
    return true;
}
static inline bool str_equal_sv(const String* self, const StringView other) {
    if (self->len != other.len)
        return false;
    for (size_t i = 0; i < self->len; i++) {
        if (str_at(self, i) != sv_at(other, i)) {
            return false;
        }
    }
    return true;
}

static inline void str_prepend_ch(String* self, const char ch) {
    str_reserve(self, self->len + 1);
}
static inline void str_append_ch(String* self, const char ch) {
    size_t pos = self->len;
    str_reserve(self, self->len + 1);
    self->len++;
    str_set(self, pos, ch);
}
static constexpr size_t STR_NOT_FOUND = (size_t)0 - 1;

// starting at `from`, return the first index of `ch`. `STR_NOT_FOUND` if nothing.
static inline size_t str_find(String* self, const char ch, const size_t from) {
    for (size_t i = from; i < self->len; i++) {
        if (str_at(self, i) == ch)
            return i;
    }
    return STR_NOT_FOUND;
}
// May trigger an allocation. All Strings should be str_delete'd because any that are >26 chars will
// be heap allocated, and MUST be deleted (else a leak)
static inline String str_make_empty(void) {
    return (String){
        .isShort = true,
        .isNull = false,
        .len = 0,
        .short_data = {},
    };
}
// **FREES THE BUFFER! This should only take malloced strings which are ready to be freed.**
[[nodiscard("Returns an allocated buffer.")]]
static inline String str_make_cstr(char* cstr) {
    String res = {};
    size_t len = strlen(cstr);
    if (len <= SSO_CAP) {
        res.isShort = true;
        res.len = len;
        if (len > 0) {
            memcpy(res.short_data, cstr, len);
        }
    } else {
        res.isShort = false;
        res.data = calloc(len, sizeof(char));
        memcpy(res.data, cstr, len);
    }
    res.len = len;
    res.isNull = false;
    free(cstr);
    return res;
}
[[nodiscard("Returns an allocated buffer.")]]
static inline String str_make_cstr_static(const char* cstr) {
    String res = {};
    size_t len = strlen(cstr);
    if (len <= SSO_CAP) {
        res.isShort = true;
        res.len = len;
        if (len > 0) {
            memcpy(res.short_data, cstr, len);
        }
    } else {
        res.isShort = false;
        res.data = calloc(len, sizeof(char));
        memcpy(res.data, cstr, len);
    }
    res.len = len;
    res.isNull = false;
    return res;
}
static inline String str_make(StringView sv) {
    String res = {};
    if (sv.len <= SSO_CAP) {
        res.isShort = true;
        res.len = sv.len;
        memcpy(res.short_data, sv.ptr, sv.len);
    } else {
        res.isShort = false;
        res.data = calloc(sv.len, sizeof(char));
        memcpy(res.data, sv.ptr, sv.len);
    }
    res.len = sv.len;
    res.isNull = false;
    return res;
}
static inline void str_delete(String* str) {
    if (str->isShort) {
        // no free needed
        memset(str->short_data, 'X', SSO_CAP);
    } else {
        memset(str->data, 'X', str->len);
        free(str->data);
        str->len = -1;
    }
    str->isNull = true;
}
//  `start` is **inclusive**
//  `end`   is **inclusive**
static inline StringView str_slice(String* self, size_t start, size_t end) {
    // hello_worl
    // 0123456789
    return (StringView){
        .ptr = str_ptr(self) + start,
        .len = end - start + 1,
    };
}
//  `start` is **inclusive**
//  `end`   is **exclusive**
static inline StringView str_slice_ex(String* self, size_t start, size_t end) {
    // hello_worl
    // 0123456789
    if (end == STR_NOT_FOUND) {
        end = self->len;
    }
    return (StringView){
        .ptr = str_ptr(self) + start,
        .len = end - start,
    };
}
static inline char* str_cstr_buf(String* self, char* buf) {
    snprintf(buf, self->len + 1, "%s", str_ptr(self));
    // BUG: leak, only for debugging so should be fine
    return buf;
}
static inline char* str_cstr(String* self) {
    char* cstr = calloc(self->len + 1, 1);
    snprintf(cstr, self->len + 1, "%s", str_ptr(self));
    // BUG: leak, only for debugging so should be fine
    return cstr;
}
static inline bool str_hasPrefix(String* self, const StringView prefix) {
    for (size_t i = 0; i < prefix.len; i++) {
        if (str_at(self, i) != sv_at(prefix, i)) {
            return false;
        }
    }
    return true;
}
static inline StringView* str_splitOnEach(String* self, const char delim, size_t* OP_count) {
    // for each instance of delim, split
    // special case: start=end, no entry.
    //    LOG_EXPR(self);
    size_t     start = 0;
    size_t     end = 0;
    StringView buf[BUF_SZ];
    size_t     count = 0;
    while (end <= self->len) {
        end = str_find(self, delim, start);
        buf[count++] = str_slice_ex(self, start, end);
        start = end + 1;
    }
    *OP_count = count;
    StringView* res = calloc(count, sizeof(StringView));
    memcpy(res, buf, sizeof(StringView) * count);
    return res;
}
static inline void str_print(String* self) {
    printf("%.*s\n", (int)self->len, str_ptr(self));
}
static inline bool str_prepend_sv(String* self, const StringView sv) {
    str_reserve(self, self->len + sv.len);
    void* dest = str_ptr(self) + sv.len;  // move everything forward by one
    void* src = str_ptr(self);            // move everything forward by one
    memmove(dest, src, self->len);
    self->len += sv.len;
    memmove(str_ptr(self), sv.ptr, sv.len);

    return true;
}
static inline bool str_append_sv(String* self, const StringView sv) {
    str_reserve(self, self->len + sv.len);
    for (size_t i = 0; i < sv.len; i++) {
        str_append_ch(self, sv_at(sv, i));
    }
    return true;
}
