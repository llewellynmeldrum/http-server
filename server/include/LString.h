#pragma once
#include <assert.h>

#include "StringView.h"
static constexpr size_t STR_NOT_FOUND = (size_t)0 - 1;

static constexpr size_t STR_END = (size_t)0 - 1;
struct String {
    char*  ptr;
    size_t cap;
    size_t len;
    bool   isNull;
};

typedef struct String String;

static const String NULL_STRING = {
    .isNull = true,
};

static inline bool str_reserve(String* self, size_t new_cap) {
    self->ptr = realloc(self->ptr, new_cap);
    self->cap = new_cap;
    return true;
}

static inline char* str_end(String* self) {
    return self->ptr + self->len;
}

static inline char str_at(const String* self, size_t n) {
    assert(self);
    assert(n < self->len);
    return self->ptr[n];
}

static inline char str_last(String* self) {
    return str_at(self, self->len - 1);
}

// **ASSUMES `pos` IS IN BOUNDS!**
// (does **NOT** perform a `str_reserve()`)
static inline void str_set(String* self, size_t pos, const char ch) {
    *(self->ptr + pos) = ch;
}

static inline void str_set_append(String* self, size_t idx, const char ch) {
    if (idx < self->len) {
        str_set(self, idx, ch);
        return;
    }
    // else, we have to extend
    size_t pos = self->len;
    str_reserve(self, self->len + 1);
    self->len++;
    str_set(self, pos, ch);
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

// starting at `from`, return the first index of `ch`. `STR_NOT_FOUND` if nothing.
static inline size_t str_find(String* self, const char ch, const size_t from) {
    for (size_t i = from; i < self->len; i++) {
        if (str_at(self, i) == ch)
            return i;
    }
    return STR_NOT_FOUND;
}
static inline size_t str_rfind(String* self, const char ch) {
    for (size_t i = self->len - 1; i >= 0; i--) {
        if (str_at(self, i) == ch)
            return i;
    }
    return STR_NOT_FOUND;
}
// May trigger an allocation. All Strings should be str_delete'd because any that are >26 chars will
// be heap allocated, and MUST be deleted (else a leak)
static inline String str_make_empty(void) {
    return (String){
        .isNull = false,
        .len = 0,
    };
}
[[nodiscard("Returns an allocated buffer.")]]
static inline String str_make_cstr(char* cstr) {
    String res = {};
    size_t len = strlen(cstr);
    res.ptr = calloc(len, sizeof(char));
    memcpy(res.ptr, cstr, len);
    res.len = len;
    res.isNull = false;
    return res;
}
static inline String str_make(StringView sv) {
    String res = {};
    res.ptr = calloc(sv.len, sizeof(char));
    memcpy(res.ptr, sv.ptr, sv.len);
    res.len = sv.len;
    res.isNull = false;
    return res;
}
// # Delete in the c++ meaning -> free
// Clears `self`'s contents, **freeing** its buffer, and **setting it to null**
static inline void str_delete(String* self) {
    memset(self->ptr, '\0', self->len);
    free(self->ptr);
    self->len = -1;
    self->isNull = true;
}
// Clears `self`'s contents, **without freeing** its buffer, and **without setting it to null**
static inline void str_clear(String* self, int num) {
    memset(self->ptr, '\0', self->len);
}

//  `start` is **inclusive**
//  `end`   is **inclusive**
static inline StringView str_slice(String* self, size_t start, size_t end) {
    if (end == STR_END) {
        end = self->len - 1;
    }
    return (StringView){
        .ptr = self->ptr + start,
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
        .ptr = self->ptr + start,
        .len = end - start,
    };
}
static inline char* str_cstr_buf(String* self, char* buf) {
    snprintf(buf, self->len + 1, "%.*s", (int)self->len, self->ptr);
    return buf;
}
static inline char* str_cstr(String* self) {
    char* cstr = calloc(self->len + 1, 1);
    snprintf(cstr, self->len + 1, "%.*s", (int)self->len, self->ptr);
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
    printf("%.*s\n", (int)self->len, self->ptr);
}

static inline bool str_prepend_sv(String* self, const StringView sv) {
    str_reserve(self, self->len + sv.len);
    void* dest = self->ptr + sv.len;  // move everything forward by one
    void* src = self->ptr;            // move everything forward by one
    memmove(dest, src, self->len);
    self->len += sv.len;
    memmove(self->ptr, sv.ptr, sv.len);

    return true;
}
static inline bool str_append_sv(String* self, const StringView sv) {
    str_reserve(self, self->len + sv.len);
    for (size_t i = 0; i < sv.len; i++) {
        str_append_ch(self, sv_at(sv, i));
    }
    return true;
}
static inline bool str_append_cstr(String* self, const char* cstr) {
    str_reserve(self, self->len + strlen(cstr));
    for (size_t i = 0; i < strlen(cstr); i++) {
        str_append_ch(self, cstr[i]);
    }
    return true;
}
static inline bool str_append_str(String* self, String* other) {
    str_reserve(self, self->len + other->len);
    for (size_t i = 0; i < other->len; i++) {
        printf("[%zu]%c,\n", i, str_at(other, i));
        str_append_ch(self, str_at(other, i));
    }
    return true;
}
// Creates a **short** string containing the **base-10** digits of `num`
static inline String str_itos(int num) {
    String           res = str_make_empty();
    constexpr size_t MAX_DIGITS = 10;
    int              copy = num;
    size_t           dig_count = 0;
    while (copy > 0) {
        copy /= 10;
        dig_count++;
    }
    if (dig_count >= MAX_DIGITS) {
        return str_make_cstr("What the fuck are you doing that for");
    }
    if (num == 0) {
        str_reserve(&res, 1);
        str_set(&res, 0, '0');
        return res;
    }
    res.len = dig_count;
    str_reserve(&res, dig_count);

    for (size_t i = 0; i < dig_count; i++) {
        // construct it backwards
        size_t idx = dig_count - i - 1;
        char   dig = '0' + (num % 10);
        str_set(&res, idx, dig);
        num /= 10;
    }

    return res;
}
