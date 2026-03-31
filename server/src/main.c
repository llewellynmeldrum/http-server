#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ByteStream.h"
#include "HTTP_Metadata.h"
#include "StringView.h"

#include "http_consts.h"
#include "http_response.h"
#include "logger.h"
#include "myutils.h"
#include "native_timer.h"
#include "server_types.h"
#include "socket_helper.h"
#include "types.h"

static const unsigned int HTTP_TCP_PORT = 80;

static ServerStats  stats = (ServerStats){};
static ServerConfig config;

static void handle_SIGINT(int sig);
static void init_config(int argc, char** argv);
static void init_socket();
static void init_threads();
static void init_responders();

static void* handle_client(void* arg);

u64  program_epoch_ns;
char document_root[PATH_MAX] = {};

int main(int argc, char** argv) {
    signal(SIGINT, handle_SIGINT);
    program_epoch_ns = get_current_ns();

    init_config(argc, argv);
    init_socket();
    init_threads();
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

typedef enum {
    HttpRequestHeaderHost,
    HttpRequestHeaderConnection,
    HttpRequestHeaderContent_Length,
    HttpRequestHeader__COUNT,
} HttpRequestHeader;

// headers are case insensitive
const static char* HttpRequestHeader_toStr[] = {
    "host",
    "connection",
    "content-length",
};

typedef struct {
    HttpRequestMethod method;
    StringView        target_sv;
    StringView        query_sv;
    HttpVersion       version;
    StringView        headers[HttpRequestHeader__COUNT];
} HttpRequest;

HttpRequestMethod parse_HttpRequestMethod(const StringView sv) {
    for (HttpRequestMethod i = 0; i < HttpRequestMethod__COUNT; i++) {
        const char* method_str = HttpRequestMethod_toStr[i];
        if (streq(method_str, sv.ptr)) {
            return i;
        }
    }
    return HttpRequestMethod_PARSE_ERROR;
}
HttpRequest parse_HttpRequest(ByteStream* stream) {
    HttpRequest res = {};
    StringView  method_sv = bs_consumeWord(stream);
    res.method = parse_HttpRequestMethod(method_sv);

    (void)bs_consumeWhitespace(stream);

    const StringView target_delims_sv = sv_make("? ");
    res.target_sv = bs_consumeUntilAnyDelim(stream, target_delims_sv);

    StringView query_sv = sv_make(nullptr);
    if (bs_peek(stream) == '?') {
        query_sv = bs_consumeWord(stream);
    }
    (void)bs_consumeWhitespace(stream);

    const StringView whitespace_sv = sv_make(" \t\r\n");
    StringView       version_sv = bs_consumeUntilAnyDelim(stream, whitespace_sv);
    // NOTE: We dont parse the body of any request. None of the request methods we support have
    // NOTE: We dont parse the body of any request. None of the request methods we support have
    // anything meaningful in their body's, if they even have them.
    return res;
}

void* handle_client(void* arg) {
    Byte buf[BUF_SZ] = {};

    Byte*  fungus = {};
    size_t good_size = arrlen(buf);
    size_t bad_size = arrlen(fungus);

    FileDescriptor client_fd = *(int*)arg;

    size_t nbytes_received = recv(client_fd, buf, arrlen(buf), 0);
    if (nbytes_received == 0) {
        LOG_FATAL("Connection made with client, but 0 bytes receieved.\n");
        LOG_ERRNO();
        return nullptr;
    }

    LOG_INFO("Recieved %zuB from client.", nbytes_received);
    ByteStream stream = bs_make(buf, nbytes_received);

    LOG_DEBUG("Parsing http request...");
    HttpRequest http_request = parse_HttpRequest(&stream);
    // do some testing, with some dummy input headers. Just provide raw data

    /*
    TODO:  start here! goto
    Resource     resource = resolveRequest(http_request);
    HttpResponse response = generateResponse(resource);
    Byte*        outgoing_data = encodeResponse(response);
    */

    size_t bytes_sent = send(client_fd, nullptr, NULL, 0);
    if (bytes_sent == SOCK_ERR) {
        LOG_FATAL("Failed to send().\n");
        return nullptr;
    } else {
        LOG_INFO("Succesfully sent (%zu) bytes in response.\n", bytes_sent);
    }
}

// sigint handler
void handle_SIGINT(int sig) {
    LOG_FATAL("<- handling SIGINT.\n");
    int opt = 1;
    setsockopt(config.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG_EXIT(EXIT_SUCCESS);
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

void init_threads(void) {
    pthread_mutex_init(&stats_mutex, nullptr);
}
