#ifndef HTTP_CONSTS_H
#define HTTP_CONSTS_H

#include <stdbool.h>
#include <stddef.h>
#define SOCK_QUEUE_LIMIT 10
#define BUF_SZ 1024
#define REQUIRED_HTTP_VER_STR "1.1"
#define MAX_FILENAME_SZ 255
#define HTTP_VERSION "1.1"
typedef struct http_response {
  void *data;
  size_t len;
} http_response;

typedef enum {
  HTTP_GET,
  HTTP_HEAD,
  HTTP_METHOD_ERR,
} http_method;

typedef enum {
  HTTP_VER_ERR,
  HTTP_VER_0_9,
  HTTP_VER_1_0,
  HTTP_VER_1_1,
  HTTP_VER_2,
  HTTP_VER_3,
} http_version;

typedef struct HTTP_TARGET {
  char *filename;
  char *mime_type;
} http_target;

typedef struct HTTP_FIELD {
  char *name;
  char *value;
} http_field;

typedef struct http_request {
  char *data;
  size_t data_len;
  http_method method;
  http_target target;
  http_version version;
  http_field *fields;
  size_t nfields;
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
  size_t body_size;
} http_response_cfg;

extern unsigned short PORT;

extern const http_response_cfg HTTP_ERR_BAD_REQUEST;
extern const http_response_cfg HTTP_ERR_NOT_FOUND;

#endif // HTTP_CONSTS_H
