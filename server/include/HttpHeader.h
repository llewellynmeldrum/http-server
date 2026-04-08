#include "StringView.h"

static inline const char* header_toStr(const HttpHeader header) {
    char* cstr = calloc(BUF_SZ, 1);
    snprintf(cstr, BUF_SZ, "(%s)=(%s)", sv_cstr(header.name), sv_cstr(header.value));
    return cstr;
}

static inline const char* header_array_ToStr(const HttpHeader* header_array, size_t sz) {
    size_t total_len = {};
    for (size_t i = 0; i < sz; i++) {
        total_len += header_array[i].name.len + 7 + header_array[i].value.len;
        // {[<>,<>],\n}
    }
    char* cstr = calloc(total_len, 1);
    strncat(cstr, "{\n", total_len);
    for (size_t i = 0; i < sz; i++) {
        char buf[BUF_SZ] = {};
        snprintf(buf, total_len, "\t\t[%zu]:%s\n", i, header_toStr(header_array[i]));
        strncat(cstr, buf, total_len);
    }
    strncat(cstr, "\t}", total_len);
    return cstr;
}
static const HttpHeader NULL_HTTPHEADER = {};
