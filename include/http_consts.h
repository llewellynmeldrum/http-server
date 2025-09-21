#ifndef HTTP_CONSTS_H
#define HTTP_CONSTS_H


#include <stdbool.h>
#include <sys/_types/_ssize_t.h>
#define SOCK_QUEUE_LIMIT 10
#define BUF_SZ 1024
#define REQUIRED_HTTP_VER_STR "1.1"
#define MAX_FILENAME_SZ 255
typedef struct http_response {
	void *data;
	ssize_t len;
} http_response;

typedef enum {
	HTTP_GET,
	HTTP_HEAD,
	HTTP_METHOD_ERR,
} HTTP_METHOD;

typedef enum {
	HTTP_VER_ERR,
	HTTP_VER_0_9,
	HTTP_VER_1_0,
	HTTP_VER_1_1,
	HTTP_VER_2,
	HTTP_VER_3,
} HTTP_VERSION;

typedef struct HTTP_TARGET {
	char *filename;
	char *mime_type;
} HTTP_TARGET;

typedef struct HTTP_FIELD {
	char *name;
	char *value;
} HTTP_FIELD;


typedef struct http_request {
	char *data;
	ssize_t data_len;
	HTTP_METHOD method;
	HTTP_TARGET target;
	HTTP_VERSION version;
	HTTP_FIELD *fields;
	ssize_t nfields;
} http_request;

void destroy_request(http_request req);


typedef struct http_resp_cfg {
	char *version;
	int status;
	char *reason_phrase;
	char *mime_type;
	char *msg_body;
	bool malloced_body;
	bool body_is_txt;
	ssize_t body_size;
} http_response_cfg;

extern unsigned short PORT;

extern const http_response_cfg HTTP_ERR_BAD_REQUEST;
extern const http_response_cfg HTTP_ERR_NOT_FOUND;

#endif //HTTP_CONSTS_H
