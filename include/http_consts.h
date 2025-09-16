#ifndef HTTP_CONSTS_H
#define HTTP_CONSTS_H

#include "http_response.h"

#define SOCK_QUEUE_LIMIT 10
#define BUF_SZ 1024
#define HTTP_VERSION "1.1"

extern unsigned short PORT;

const http_response_cfg HTTP_ERR_BAD_REQUEST;
const http_response_cfg HTTP_ERR_NOT_FOUND;

#endif //HTTP_CONSTS_H
