#pragma once
#include <netinet/in.h>
#include <stdbool.h>

typedef int                FileDescriptor;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr    sockaddr;

typedef struct ServerConfig {
    bool           is_localhost;
    char           hostname[16];
    unsigned short port;
    sockaddr_in    sock_addr;
    FileDescriptor server_fd;
} ServerConfig;
