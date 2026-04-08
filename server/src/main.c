#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ByteStream.h"
#include "Globals.h"
#include "HttpError.h"
#include "HttpMetadata.h"
#include "HttpParsing.h"
#include "HttpTarget.h"
#include "LString.h"
#include "ServerTypes.h"
#include "SocketWrapper.h"
#include "StringMap.h"
#include "StringView.h"

#include "HttpRequest.h"

pthread_t                 client_handler_thread;
static const unsigned int HTTP_TCP_PORT = 80;

#define LOGGER_DISABLE_TIMER

#include "Logger.h"

#include "CWrappers.h"
static ServerConfig config;

static void handle_SIGINT(int sig);
static void init_config(int argc, char** argv);
static void init_socket();
static void init_responders();

static void* handle_client(void* arg);

static void init_static_data(void) {
    init_percent_valmap();
}
u64  program_epoch_ns;
char document_root[PATH_MAX] = {};

int main(int argc, char** argv) {
    signal(SIGINT, handle_SIGINT);
    program_epoch_ns = get_current_ns();
    init_static_data();

    handle_client((void*)0);
    exit(EXIT_SUCCESS);

    init_config(argc, argv);
    init_socket();
    init_responders();

    bool accepting_connections = true;
    while (accepting_connections) {
        sockaddr_in client_addr;
        socklen_t   client_addr_len = sizeof(client_addr);
        // BUG: Why is this a heap allocation?
        FileDescriptor* client = malloc(sizeof(FileDescriptor));
        LOG_INFO(FSTR_AVALIABLE_AT, config.port, config.hostname, config.port);
        if (!client) {
            LOG_FATAL("Unable to allocate memory for client FD!");
            LOG_EXIT(EXIT_FAILURE);
        }
        *client = accept(config.server_fd, (sockaddr*)&client_addr, &client_addr_len);
        if (*client == SOCK_ERR) {
            LOG_FATAL("Unable to accept client connection:");
            LOG_EXIT(EXIT_FAILURE);
        }

        // dispatch and detach thread
        pthread_create(&client_handler_thread, nullptr, handle_client, (void*)client);
        pthread_detach(client_handler_thread);
    }

    LOG_EXIT(EXIT_SUCCESS);
}
void init_responders(void) {
    for (int i = 0; i < PATH_MAX; i++) {
        document_root[i] = '\0';
    }
    if (!realpath("../www", document_root)) {
        LOG_FATAL("Realpath failed on doc root. doc_root:%s\n", document_root);
        perror("Errno");
        LOG_EXIT(EXIT_FAILURE);
    }
}

struct HttpResponse {
    FILE*  fptr;
    String resolved_path;
    // requires the fields of a http response, like the status code, and other response line
    // details.
    //
    bool      hasError;
    HttpError err;
};
typedef struct HttpResponse HttpResponse;

HttpResponse generate_HttpResponse(HttpTarget target) {
    // given some resource (which is essentially a resolved file path on the servers machine),
    // 0. first check to see if there is already an error -> if so, generate the matching response.
    // 1. check to make sure the requested file exists -> if not, resopnse=404.
    // 2. check to make sure the file is accessible(?) -> if not, response=403
    // 3. check to make sure the file is of a reasonable size
    // 4. if all the checks pass, generate the structured HttpResponse, which will contain the
    // status code, etc, everything needed to serialize the resultant response line, headers, and
    // body.
    return (HttpResponse){};
}

// fat pointer style array
struct ByteArray {
    Byte*  ptr;
    size_t len;
};
typedef struct ByteArray ByteArray;
ByteArray                serialize_HttpResponse(HttpResponse response) {
    // given some response, serialize its fields.
    // Return a heap allocated byte array,
    return (ByteArray){};
}
void       init_percent_valmap(void);
HttpTarget resolve_HttpRequest(HttpRequest request);
void*      handle_client(void* arg) {
    // request-target = origin-form
    //
    Byte test_get_header_buf[] = "GET /?idkthisisaquery HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "User-Agent: curl/8.6.0\r\n"
                                 "Accept: */*\r\n"
                                 "\r\n";

    int n_bytes_received = arrlen(test_get_header_buf);
    LOG_INFO("Recieved %zuB from client.", n_bytes_received);

    ByteStream  stream = bs_make(test_get_header_buf, n_bytes_received);
    HttpRequest request = parse_HttpRequest(&stream);
    HttpTarget  target = resolve_HttpRequest(request);
    // TODO: implement V
    HttpResponse response = generate_HttpResponse(target);
    // TODO: implement V
    ByteArray outgoing_data = serialize_HttpResponse(response);

    // TODO: Refactor some of the header only mess ive got going on.
    //       Move each stage to a separate module (.c/.h pair)
    /*
     * INTENDED ORDER:
    HttpRequest  request  = parse_HttpRequest(&stream);
    HttpTarget resource = resolveRequest(http_request);
    HttpResponse response = generateResponse(resource);
    Byte*        outgoing_data = encodeResponse(response);
    */
    return nullptr;
}

// sigint handler
void handle_SIGINT(int sig) {
    LOG_FATAL("<- handling SIGINT.\n");
    int opt = 1;
    setsockopt(config.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG_EXIT(EXIT_SUCCESS);
    exit(EXIT_SUCCESS);
}

void init_config(int argc, char** argv) {
    int num_args = argc - 1;
    config.is_localhost = false;
    strcpy(config.hostname, "lmeldrum.dev");
    config.port = HTTP_TCP_PORT;
    config.sock_addr = (sockaddr_in){
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(HTTP_TCP_PORT),
    };

    if (num_args == 1) {
        if (streq(argv[1], "-l") || streq(argv[1], "--local")) {
            config.is_localhost = true;
        } else if (streq(argv[1], "-h") || streq(argv[1], "-?") || streq(argv[1], "--help")) {
            LOG_INFO(STR_USAGE);
            LOG_EXIT(EXIT_SUCCESS);
        }
    } else if (num_args == 0) {
        config.is_localhost = false;
    } else {
        LOG_FATAL("Unknown args passed, usage: \n %s %s\n", argv[0], STR_USAGE);
    }

    // make alterations from default
    if (config.is_localhost) {
        LOG_INFO(BOLD "Localhost mode selected! localhost:random_port will be "
                      "used." FMT_CLEAR "\n\n");
        strcpy(config.hostname, "127.0.0.1");
        config.port = 49152;
        config.sock_addr.sin_port = htons(config.port);
    }
}

void handle_socket_bind_err(SOCK_STATUS bind_status) {
    int time_to_sleep = 25;
    LOG_ERROR("Port %hu failed to bind. \n", ntohs(config.sock_addr.sin_port));
    LOG_ERRNO();
    while (bind_status == SOCK_ERR) {
        LOG_INFO("Trying again in %ds... \n", time_to_sleep);
        sleep(time_to_sleep);
        bind_status =
            bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
        time_to_sleep *= 2;
    }
}

void init_socket(void) {
    config.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (config.server_fd == SOCK_ERR) {
        LOG_FATAL("Unable to create socket.\n");
        LOG_EXIT(EXIT_FAILURE);
    }

    SOCK_STATUS bind_status =
        bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
    if (bind_status == SOCK_ERR)
        handle_socket_bind_err(bind_status);

    SOCK_STATUS listen_status = listen(config.server_fd, MAX_LISTENER_THREADS);
    if (listen_status == SOCK_ERR) {
        LOG_FATAL("Unable to start listening for connections.\n");
        LOG_EXIT(EXIT_FAILURE);
    }
}
