#include "LString.h"
#include "StringView.h"

struct HttpHeader {
    bool isRequest_t;
    union {
        struct {
            StringView name;
            StringView value;
        } request;
        struct {
            String* name;
            String* value;
        } response;
    };
};
typedef struct HttpHeader HttpHeader;
static inline HttpHeader  requestHeader_make(StringView l, StringView r) {
    return (HttpHeader){
        .isRequest_t = true,
        .request.name = l,
        .request.value = r,
    };
}
static inline HttpHeader responseHeader_make(String* l, String* r) {
    return (HttpHeader){
        .isRequest_t = false,
        .response.name = l,
        .response.value = r,
    };
}
static inline char* header_toStr(const HttpHeader header) {
    // for debugging purposes, leak is fine
    char* cstr = calloc(BUF_SZ, 1);
    if (header.isRequest_t) {
        snprintf(
            cstr, BUF_SZ, "%s : %s", sv_cstr(header.request.name), sv_cstr(header.request.value));
    } else {
        snprintf(cstr,
                 BUF_SZ,
                 "%s : %s",
                 str_cstr(header.response.name),
                 str_cstr(header.response.value));
    }
    return cstr;
}

// for debugging purposes, leak is fine
static inline const char* header_array_ToStr(const HttpHeader* header_array, size_t count) {
    size_t total_len = {};
    for (size_t i = 0; i < count; i++) {
        if (header_array[i].isRequest_t) {
            total_len += header_array[i].request.name.len + 7 + header_array[i].request.value.len;
        } else {
            total_len +=
                header_array[i].response.name->len + 7 + header_array[i].response.value->len;
        }
    }
    char cstr[BUF_SZ];
    strlcat(cstr, "{\n", BUF_SZ);
    for (size_t i = 0; i < count; i++) {
        char  buf[BUF_SZ] = {};
        char* cur_header = header_toStr(header_array[i]);
        snprintf(buf, total_len, "\t\t[%zu]:%s\n", i, cur_header);
        strlcat(cstr, buf, BUF_SZ);

        free(cur_header);
    }
    strlcat(cstr, "\t}", BUF_SZ);
    char* res = calloc(BUF_SZ, 1);
    strncpy(res, cstr, BUF_SZ - 1);
    return res;
}
static const HttpHeader NULL_HTTPHEADER = {};
