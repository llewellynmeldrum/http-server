#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_consts.h"
#include <stdbool.h>
#ifdef __APPLE__
#include <sys/syslimits.h>
#elif defined(__linux__)
#include <linux/limits.h>
#endif

HTTP_Response build_http_response(HTTP_Request request);
HTTP_Request parse_http_request(char *data, size_t datalen);
extern char document_root[PATH_MAX];
#endif // HTTP_RESPONSE_H
