#pragma once
#include "http_consts.h"
#include "types.h"

static char*         parse_mime_type(char* filename);
static HTTP_Response serialize_http_response(HTTP_ResponseConfig c);

/* we use the windows style for completeness. */
#define NEWL "\r\n"

static const size_t MAX_REQUEST_LINE_SZ =
    MAX_TARGET_STRLEN + MAX_VERSION_STRLEN + MAX_METHOD_STRLEN + 1;

static HTTP_Method  parse_method(char* method_str);
static HTTP_Target  parse_target(char* target_str);
static HTTP_Version parse_version(char* version_str);
