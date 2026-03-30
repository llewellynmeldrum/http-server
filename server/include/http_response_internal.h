#pragma once
#include "http_consts.h"
#include <stddef.h>
#include <sys/stat.h>
static const size_t MAX_FILE_SZ = (1e9); // 1Gb ~(125MiB)
static const size_t MAX_RESPONSE_HEADER_SZ = 1024;

static char *parse_mime_type(char *filename);
static char *read_file(const char *filename, size_t file_size);
static HTTP_Response serialize_http_response(HTTP_ResponseConfig c);

/* we use the windows style for completeness. */
#define NEWL "\r\n"

static inline int syscall_file_size(const char *filename) {
  struct stat s;
  stat(filename, &s);
  return s.st_size;
}
static const size_t MAX_VERSION_STRLEN = 32;
static const size_t MAX_METHOD_STRLEN = 32;
static const size_t MAX_TARGET_STRLEN = 256;

static const size_t MAX_REQUEST_LINE_SZ =
    MAX_TARGET_STRLEN + MAX_VERSION_STRLEN + MAX_METHOD_STRLEN + 1;

static const size_t MAX_HEADER_FIELD_SZ = 128;
static const size_t MAX_HEADER_VALUE_SZ = 1024;
static HTTP_Method parse_method(char *method_str);
static HTTP_Target parse_target(char *target_str);
static HTTP_Version parse_version(char *version_str);
