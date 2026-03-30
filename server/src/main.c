#include <arpa/inet.h>
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "http_consts.h"
#include "http_response.h"
#include "logger.h"
#include "myutils.h"
#include "native_timer.h"
#include "server_types.h"
#include "socket_helper.h"

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
    program_epoch_ns = get_current_ns();
    init_config(argc, argv);
    init_socket();
    init_threads();
    init_responders();

    bool accepting_connections = true;
    while (accepting_connections) {
        sockaddr_in      client_addr;
        socklen_t        client_addr_len = sizeof(client_addr);
        FILE_DESCRIPTOR* client =
            malloc(sizeof(FILE_DESCRIPTOR));  // 2. listen for client connections - accept
                                              // blocks the thread until it finds one
        LOG_INFO(FSTR_AVALIABLE_AT, config.port, config.hostname, config.port);
        if (!client) {
            LOG_FATAL("Unable to allocate memory for client FD!\n");
            LOG_EXIT(EXIT_FAILURE);
        }
        *client = accept(config.server_fd, (sockaddr*)&client_addr, &client_addr_len);
        if (*client == SOCK_ERR) {
            LOG_FATAL("Unable to accept client connection:\n");
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

void* handle_client(void* arg) {
    /* 0. SETUP, UPDATE STATS */
    char*           request_data = malloc(BUF_SZ);
    FILE_DESCRIPTOR client = *(int*)arg;
    size_t          bytes_received = recv(client, request_data, BUF_SZ, 0);
    if (bytes_received <= 0) {
        LOG_FATAL("Connection made with client, but <=0 bytes (%zu) receieved.\n", bytes_received);
        goto CLIENT_CLEANUP;
    }

    pthread_mutex_lock(&stats_mutex);
    stats.bytes_received += bytes_received;
    ++stats.requests_received;
    pthread_mutex_unlock(&stats_mutex);

    LOG_INFO("Recieved %zuB from client.\n", bytes_received);
    pretty_print_buffer("RECEIVED MESSAGE", request_data, 0);

    HTTP_Request  request = parse_http_request(request_data, bytes_received);
    HTTP_Response response = build_http_response(request);

    if (!response.data) {
        LOG_FATAL("Unable to build valid response!\n");
        goto CLIENT_CLEANUP;
    }

    size_t bytes_sent = send(client, response.data, response.len, 0);
    if (bytes_sent == SOCK_ERR) {
        LOG_FATAL("Failed to send above http response!!\n");
        goto CLIENT_CLEANUP;
    } else {
        LOG_INFO("Succesfully sent (%zu) bytes in response!\n", bytes_sent);
        pthread_mutex_lock(&stats_mutex);
        stats.bytes_sent += response.len;
        ++stats.results_sent;
        pthread_mutex_unlock(&stats_mutex);
    }
    free(response.data);

CLIENT_CLEANUP:
    close(client);
    free(request_data);
    LOG_INFO(FSTR_AVALIABLE_AT, config.port, config.hostname, config.port);
    LOG_INFO(FSTR_STATS(stats));

    return NULL;
}

// sigint handler
void handle_SIGINT(int sig) {
    LOG_INFO("<- handling SIGINT.\n");
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

#define DEBUG
void handle_socket_bind_err(SOCK_STATUS bind_status) {
#ifdef DEBUG
    while (bind_status == SOCK_ERR) {
        config.port = RAND_RANGE(49152, 65535);
        config.sock_addr.sin_port = htons(config.port);
        bind_status =
            bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
    }
#else
    int time_to_sleep = 5;
    LOG_ERROR("Port %hu failed to bind. \n", ntohs(config.sock_addr.sin_port));
    LOG_ERRNO();
    while (bind_status == SOCK_ERR) {
        LOG_INFO("Trying again in %ds... \n", time_to_sleep);
        sleep(time_to_sleep);
        bind_status =
            bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
        time_to_sleep *= 2;
    }
#endif
}

void init_socket() {
    config.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (config.server_fd == SOCK_ERR) {
        LOG_FATAL("Unable to create socket.\n");
        LOG_EXIT(EXIT_FAILURE);
    }

    SOCK_STATUS bind_status =
        bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
    if (bind_status == SOCK_ERR)
        handle_socket_bind_err(bind_status);

    SOCK_STATUS listen_status = listen(config.server_fd, SOCK_QUEUE_LIMIT);
    if (listen_status == SOCK_ERR) {
        LOG_FATAL("Unable to start listening for connections.\n");
        LOG_EXIT(EXIT_FAILURE);
    }
}

void init_threads() {
    pthread_mutex_init(&stats_mutex, nullptr);
}
