#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

#include "http_consts.h"
#include <stdbool.h>
#include <sys/_types/_ssize_t.h>

http_response build_http_response(http_request request);
http_request parse_http_request(char* data, ssize_t datalen);
#endif //HTTP_RESPONSE_H
