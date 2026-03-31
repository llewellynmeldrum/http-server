#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_consts.h"
#include "types.h"
#include <stdbool.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#elif defined(__linux__)
#include <linux/limits.h>
#endif

HTTP_Response build_http_response(HTTP_Request request);
HTTP_Request  parse_http_request(Byte data[BUF_SZ], size_t data_len);
extern char   document_root[PATH_MAX];
#endif  // HTTP_RESPONSE_H
