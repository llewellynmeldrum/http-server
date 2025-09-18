#ifndef HTTP_CONSTS_H
#define HTTP_CONSTS_H


#define SOCK_QUEUE_LIMIT 10
#define BUF_SZ 1024
#define HTTP_VERSION "1.1"

typedef struct http_resp_cfg {
	char *version;
	int status;
	char *reason_phrase;
	char *mime_type;
	char *msg_body;
	bool malloced_body;
} http_response_cfg;

extern unsigned short PORT;

extern const http_response_cfg HTTP_ERR_BAD_REQUEST;
extern const http_response_cfg HTTP_ERR_NOT_FOUND;

#endif //HTTP_CONSTS_H
