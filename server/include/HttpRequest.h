#pragma once
#include "HttpError.h"
#include "HttpMetadata.h"
#include "StringView.h"
typedef enum {
    HttpRequestMethod_GET,
    HttpRequestMethod_HEAD,
    HttpRequestMethod_OPTIONS,
    HttpRequestMethod_TRACE,
    HttpRequestMethod_PUT,      // UNSAFE!!
    HttpRequestMethod_DELETE,   // UNSAFE!!
    HttpRequestMethod_POST,     // UNSAFE!!
    HttpRequestMethod_PATCH,    // UNSAFE!!
    HttpRequestMethod_CONNECT,  // UNSAFE!!
    HttpRequestMethod__COUNT,
    HttpRequestMethod_NOT_IMPLEMENTED,
    HttpRequestMethod_BAD_REQUEST,
} HttpRequestMethod;

static const char* HttpRequestMethod_toStr[] = {
    "GET", "HEAD", "OPTIONS", "TRACE", "PUT", "DELETE", "POST", "PATCH", "CONNECT",
};

typedef enum {
    HttpVersion_1_1,
    HttpVersion_0_9,
    HttpVersion_1_0,
    HttpVersion_2,
    HttpVersion_3,
    HttpVersion__COUNT,
    HttpVersion_NOT_SUPPORTED,
} HttpVersion;

static const char* HttpVersion_toStr[] = {
    "HTTP/1.1", "HTTP/0.9", "HTTP/1.0", "HTTP/2", "HTTP/3",
};

typedef enum {
    HttpRequestHeaderHost,
    HttpRequestHeaderConnection,
    HttpRequestHeaderContent_Length,
    HttpRequestHeader__COUNT,
} HttpRequestHeader;

// headers are case insensitive
const static char* HttpRequestHeader_toStr[] = {
    "host",
    "connection",
    "content-length",
};

typedef struct {
    HttpRequestMethod method;
    StringView        target_sv;
    HttpVersion       version;
    HttpHeader*       headers;
    size_t            num_headers;
    bool              hasError;
    HttpError         err;
} HttpRequest;
