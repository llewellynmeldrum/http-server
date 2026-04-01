#include "http_consts.h"
#include "http_response.h"
#include <stdbool.h>

const HTTP_ResponseConfig HTTP_ResponseConfig_ERR_NOTFOUND = (HTTP_ResponseConfig){
    .version = HTTP_VERSION,
    .status = 404,
    .reason_phrase = "Not Found",
    .mime_type = "text/plain",
    .msg_body = "404: Not Found",
};

const HTTP_ResponseConfig HTTP_ResponseConfig_ERR_BADREQUEST = (HTTP_ResponseConfig){
    .version = HTTP_VERSION,
    .status = 400,
    .reason_phrase = "Bad Request",
    .mime_type = "text/plain",
    .msg_body = "400: Client made bad request.",
};
