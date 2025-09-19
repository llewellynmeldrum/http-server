#ifndef SERVER_TYPES_H
#define SERVER_TYPES_H

#include <netinet/in.h>
#include <stdbool.h>

typedef int FILE_DESCRIPTOR;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

typedef struct ServerConfig {
	bool is_localhost;
	char ipv4_str[16];
	unsigned short port;
	sockaddr_in sock_addr;
	FILE_DESCRIPTOR server_fd;
} ServerConfig;


#endif //SERVER_TYPES_H
