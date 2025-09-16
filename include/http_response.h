#ifndef HTTP_RESPONSE_H
#define HTTP_RESPONSE_H

typedef struct http_resp_cfg {
	char *version;
	int status;
	char *reason_phrase;
	char *mime_type;
	char *msg_body;
} http_response_cfg;

char *build_http_response(char* request_filename);
#endif //HTTP_RESPONSE_H
