#pragma once
#include <stdbool.h>
#include <stddef.h>
static const int MAX_LISTENER_THREADS = 10;

// TODO: fix names and usage
static const size_t BUF_SZ = 1024;
static const size_t MAX_VERSION_STRLEN = 32;
static const size_t MAX_METHOD_STRLEN = 32;
static const size_t MAX_TARGET_STRLEN = 256;
static const size_t MAX_FILE_SZ = (1e9);  // 1Gb ~(125MiB)
static const size_t MAX_RESPONSE_HEADER_SZ = 1024;
#define REQUIRED_HTTP_VER_STR "1.1"
#define HTTP_VERSION "1.1"

typedef struct http_response {
    void*  data;
    size_t len;
} HTTP_Response;

typedef enum {
    HTTP_GET,
    HTTP_HEAD,
    HTTP_METHOD_ERR,
} HTTP_Method;

typedef enum {
    HTTP_VER_ERR,
    HTTP_VER_0_9,
    HTTP_VER_1_0,
    HTTP_VER_1_1,
    HTTP_VER_2,
    HTTP_VER_3,
} HTTP_Version;

typedef struct HTTP_TARGET {
    char* filename;
    char* mime_type;
} HTTP_Target;
static const HTTP_Target NULL_TARGET = {};

typedef struct HTTP_FIELD {
    char* name;
    char* value;
} HTTP_Field;

typedef struct http_request {
    char*        data;
    size_t       data_len;
    HTTP_Method  method;
    HTTP_Target  target;
    HTTP_Version version;
    HTTP_Field*  fields;
    size_t       nfields;
} HTTP_Request;
extern const HTTP_Request HTTP_Request_ERR_NOTFOUND;

typedef struct http_resp_cfg {
    char*  version;
    int    status;
    char*  reason_phrase;
    char*  mime_type;
    char*  msg_body;
    bool   malloced_body;
    bool   body_is_txt;
    size_t body_size;
} HTTP_ResponseConfig;

extern unsigned short PORT;

extern const HTTP_ResponseConfig HTTP_ResponseConfig_ERR_BADREQUEST;
extern const HTTP_ResponseConfig HTTP_ResponseConfig_ERR_NOTFOUND;
