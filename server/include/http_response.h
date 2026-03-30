#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_consts.h"
#include <stdbool.h>

http_response build_http_response(http_request request);
http_request parse_http_request(char *data, size_t datalen);
#endif // HTTP_RESPONSE_H
