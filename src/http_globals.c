#include "http_response.h"
#include "http_consts.h"
#include <stdbool.h>

const http_response_cfg HTTP_ERR_NOT_FOUND = (http_response_cfg) {
	.version = HTTP_VERSION,
	.status = 404,
	.reason_phrase = "Not Found",
	.mime_type = "text/plain",
	.msg_body = "404: Not Found",
};

const http_response_cfg HTTP_ERR_BAD_REQUEST = (http_response_cfg) {
	.version = HTTP_VERSION,
	.status = 400,
	.reason_phrase = "Bad Request",
	.mime_type = "text/plain",
	.msg_body = "400: Client made bad request.",
};
