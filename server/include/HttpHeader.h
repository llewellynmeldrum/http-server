#include "LString.h"
#include "StringView.h"

struct HttpHeader {
    bool isRequest_t;
    union {
        struct {
            StringView request_name;
            StringView request_val;
        };
        struct {
            String response_name;
            String response_val;
        };
    };
};
typedef struct HttpHeader HttpHeader;
static inline HttpHeader  requestHeader_make(StringView l, StringView r) {
    return (HttpHeader){
        .isRequest_t = true,
        .request_name = l,
        .request_val = r,
    };
}
static inline HttpHeader responseHeader_make(String l, String r) {
    return (HttpHeader){
        .isRequest_t = false,
        .response_name = l,
        .response_val = r,
    };
}
static inline char* header_toStr(HttpHeader header) {
    // for debugging purposes, leak is fine
    char* cstr = calloc(BUF_SZ, 1);
    if (header.isRequest_t) {
        snprintf(
            cstr, BUF_SZ, "%s : %s", sv_cstr(header.request_name), sv_cstr(header.request_val));
    } else {
        snprintf(cstr,
                 BUF_SZ,
                 "%s : %s",
                 str_cstr(&header.response_name),
                 str_cstr(&header.response_val));
    }
    return cstr;
}

// for debugging purposes, leak is fine
static inline const char* header_array_ToStr(const HttpHeader* header_array, size_t count) {
    size_t total_len = {};
    for (size_t i = 0; i < count; i++) {
        if (header_array[i].isRequest_t) {
            total_len += header_array[i].request_name.len + 7 + header_array[i].request_val.len;
        } else {
            total_len += header_array[i].response_name.len + 7 + header_array[i].response_val.len;
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
