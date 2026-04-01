#include <arpa/inet.h>
#include <limits.h>
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
#include "HttpMetadata.h"
#include "Logger.h"
#include "ServerTypes.h"
#include "SocketWrapper.h"
#include "StringView.h"

#include "HttpRequest.h"

pthread_t                 client_handler_thread;
static const unsigned int HTTP_TCP_PORT = 80;

static ServerConfig config;

static void handle_SIGINT(int sig);
static void init_config(int argc, char** argv);
static void init_socket();
static void init_responders();

static void* handle_client(void* arg);

u64  program_epoch_ns;
char document_root[PATH_MAX] = {};

int main(int argc, char** argv) {
    signal(SIGINT, handle_SIGINT);
    program_epoch_ns = get_current_ns();
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

HttpRequestMethod parse_HttpRequestMethod(const StringView sv) {
    for (HttpRequestMethod i = 0; i < HttpRequestMethod__COUNT; i++) {
        const char* method_str = HttpRequestMethod_toStr[i];
        if (sv_matchesStr(sv, method_str)) {
            return i;
        }
    }
    return HttpRequestMethod_PARSE_ERROR;
}

HttpVersion parse_HttpVersion(const StringView sv) {
    for (HttpVersion i = 0; i < HttpVersion__COUNT; i++) {
        const char* version_str = HttpVersion_toStr[i];
        if (sv_matchesStr(sv, version_str)) {
            return i;
        }
    }
    return HttpVersion__PARSE_ERROR;
}
HttpRequest parse_HttpRequest(ByteStream* stream) {
    bs_debugLog(stream);
    HttpRequest res = {};
    StringView  method_sv = bs_consumeWord(stream);
    sv_print(method_sv);
    res.method = parse_HttpRequestMethod(method_sv);

    (void)bs_consumeWhitespace(stream);

    const StringView target_delims_sv = sv_make("? ");
    sv_print(target_delims_sv);
    res.target_sv = bs_consumeUntilAnyDelim(stream, target_delims_sv);
    sv_print(res.target_sv);

    StringView query_sv = sv_make(nullptr);
    if (bs_peek(stream) == '?') {
        (void)bs_consumeWhitespace(stream);
        query_sv = bs_consumeUntilDelim(stream, ' ');
    }
    sv_print(res.query_sv);
    res.query_sv = query_sv;
    (void)bs_consumeWhitespace(stream);

    const StringView newline_sv = sv_make("\r\n");
    StringView       version_sv = bs_consumeUntilAnyDelim(stream, newline_sv);
    sv_print(version_sv);
    res.version = parse_HttpVersion(version_sv);

    const StringView endOfLine = bs_consumeN(stream, 2);
    sv_print(endOfLine);
    if (!sv_matchesStr(endOfLine, "\r\n")) {
        // BUG: no CRLF detected after method line
        return res;
    }
    // until there is an empty line, parse like there is a header
    const StringView header_delims_sv = sv_make(" \r\n:");
    StringViewPair   svp_buf[MAX_HEADER_COUNT] = {};
    while (!(bs_peek(stream) == '\r' && bs_peek2(stream) == '\n')) {
        // 1. consume until : or \n or \r
        // if it was a :, consume until newline. That is your header val
        StringView lhs = bs_consumeUntilAnyDelim(stream, header_delims_sv);
        sv_print(lhs);
        if (bs_peek(stream) != ':') {
            // BUG:  Malformed header field
            break;
        }
        (void)bs_consume(stream);  // skip ':'
        StringView rhs = bs_consumeUntilAnyDelim(stream, newline_sv);
        rhs = sv_strip(rhs, header_delims_sv);
        sv_print(rhs);
        svp_buf[res.num_headers++] = svp_make(lhs, rhs);
        (void)bs_consumeN(stream, 2);  // skip ':'
    }
    res.headers = calloc(res.num_headers, sizeof(StringViewPair));
    memcpy(res.headers, svp_buf, res.num_headers * sizeof(StringViewPair));
    // TODO: fix Printing of headers, parsing seems to be working
    //
    //
    //
    //
    // NOTE: We dont parse the body or headers atm.
    return res;
}

void* handle_client(void* arg) {

    //    FileDescriptor client_fd = *(int *)arg;

    Byte test_get_header_buf[] = "GET /contact?idkthisisaquery HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "User-Agent: curl/8.6.0\r\n"
                                 "Accept: */*\r\n"
                                 "\r\n";
    int  n_bytes_received = arrlen(test_get_header_buf);
    LOG_INFO("Recieved %zuB from client.", n_bytes_received);

    ByteStream stream = bs_make(test_get_header_buf, n_bytes_received);
    LOG_DEBUG("Parsing http request...");
    HttpRequest http_request = parse_HttpRequest(&stream);
    LOG_DEBUG("Parsed http request:");
    LOG_EXPR(http_request);

    (void)http_request;
    // do some testing, with some dummy input headers. Just provide raw data

    /*
    Resource     resource = resolveRequest(http_request);
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
