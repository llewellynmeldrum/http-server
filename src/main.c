#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "http_consts.h"
#include "http_response.h"
#include "socket_helper.h"
#include "server_types.h"
#include "myutils.h"

static const char *EC2_INSTANCE_IPV4 =  "3.105.0.153";
static const unsigned int HTTP_TCP_PORT = 80;

static ServerStats stats = (ServerStats) { };
static ServerConfig config;

static void handle_SIGINT();
static void init_config(int argc, char** argv);
static void init_socket();
static void init_threads();

static void *handle_client(void* arg);

int main(int argc, char** argv) {
	signal(SIGINT, handle_SIGINT);

	init_config(argc, argv);
	init_socket();
	init_threads();

	bool accepting_connections = true;
	while(accepting_connections) {
		sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		FILE_DESCRIPTOR * client = malloc(sizeof(FILE_DESCRIPTOR)); // 2. listen for client connections - accept blocks the thread until it finds one
		log(FSTR_AVALIABLE_AT(config));
		if (!client) logfatal_exit("Unable to allocate memory for client FD!\n");
		*client = accept(config.server_fd, (sockaddr*)&client_addr, &client_addr_len);
		if (*client == SOCK_ERR) {
			logfatalerrno("Unable to accept client connection:\n");
			free(client);
			logexit(EXIT_FAILURE);
		}

		// dispatch and detach thread
		pthread_create(&client_handler_thread, NULL, handle_client, (void*)client);
		pthread_detach(client_handler_thread);
	}

	logexit(EXIT_SUCCESS);
}


void *handle_client(void* arg) {
	/* 0. SETUP, UPDATE STATS */
	char *request_data = malloc(BUF_SZ);
	FILE_DESCRIPTOR client = *(int*)arg;
	ssize_t bytes_received = recv(client, request_data, BUF_SZ, 0);
	if (bytes_received <= 0) {
		logfatal("Connection made with client, but <=0 bytes (%zu) receieved.\n", bytes_received);
		goto CLIENT_CLEANUP;
	}

	pthread_mutex_lock(&stats_mutex);
	stats.bytes_received += bytes_received;
	++stats.requests_received;
	pthread_mutex_unlock(&stats_mutex);

	log("Recieved %zuB from client.\n", bytes_received);
	pretty_print_buffer("RECEIVED MESSAGE", request_data, 0);


	http_request request = parse_http_request(request_data, bytes_received);
	http_response response = build_http_response(request);

	if (!response.data) {
		logfatal("Unable to build valid response!\n");
		goto CLIENT_CLEANUP;
	}

	ssize_t bytes_sent = send(client, response.data, response.len, 0);
	if (bytes_sent == SOCK_ERR) {
		logfatalerrno("Failed to send above http response!!\n");
	} else {
		log("Succesfully sent (%zu) bytes in response!\n", bytes_sent);
		pthread_mutex_lock(&stats_mutex);
		stats.bytes_sent += response.len;
		++stats.results_sent;
		pthread_mutex_unlock(&stats_mutex);
	}
	free(response.data);

CLIENT_CLEANUP:
	close(client);
	free(request_data);
	log(FSTR_AVALIABLE_AT(config));
	log(FSTR_STATS(stats));

	return NULL;
}

void handle_SIGINT() {
	log("<- handling SIGINT.\n");
	int opt = 1;
	setsockopt(config.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	logexit(EXIT_SUCCESS);
}

void init_config(int argc, char** argv) {
	int num_args = argc - 1;
	// set defaults
	config.is_localhost = false,
	strcpy(config.ipv4_str, EC2_INSTANCE_IPV4);
	config.port = HTTP_TCP_PORT,
	config.sock_addr = (sockaddr_in) {
		.sin_family 		= AF_INET,
		 .sin_addr.s_addr 	= INADDR_ANY,
		  .sin_port 		= htons(HTTP_TCP_PORT),
	};

	// check args
	switch (num_args) {
	case 1:
		if (streq(argv[1], "-l") || streq(argv[1], "--local")) {
			config.is_localhost = true;
		} else if (streq(argv[1], "-h") || streq(argv[1], "-?") || streq(argv[1], "--help")) {
			log(STR_USAGE);
			logexit(EXIT_SUCCESS);
		}
		break;
	case 0:
		config.is_localhost = false;
		break;
	default:
		logfatal_exit("Unknown args passed, usage: \n %s %s\n", argv[0], STR_USAGE);
		break;
	}


	// make alterations from default
	if (config.is_localhost) {
		log(SET_BOLD "Localhost mode selected! localhost:random_port will be used." SET_CLEAR "\n\n");
		strcpy(config.ipv4_str, "127.0.0.1");
		config.port = 49152;
		config.sock_addr.sin_port = htons(config.port);
	}

}

void handle_socket_bind_err(SOCK_STATUS bind_status) {
	log("looking\n");
	if (config.is_localhost) {
		while(bind_status == SOCK_ERR) {
			config.port = RAND_RANGE(49152, 65535);
			config.sock_addr.sin_port = htons(config.port);
			bind_status = bind(config.server_fd, (sockaddr * )&config.sock_addr, sizeof(config.sock_addr));
		}
	} else {
		int time_to_sleep = 5;
		log("Port %hu failed to bind. \n", config.sock_addr.sin_port);
		while (bind_status == SOCK_ERR) {
			log("Trying again in %ds... \n", time_to_sleep);
			logerrno();
			sleep(time_to_sleep);
			bind_status = bind(config.server_fd, (sockaddr * )&config.sock_addr, sizeof(config.sock_addr));
			time_to_sleep *= 2;
		}
	}
}

void init_socket() {
	config.server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (config.server_fd == SOCK_ERR) logfatal_exit("Unable to create socket.\n");

	SOCK_STATUS bind_status = bind(config.server_fd, (sockaddr * )&config.sock_addr, sizeof(config.sock_addr));
	if (bind_status == SOCK_ERR) handle_socket_bind_err(bind_status);

	SOCK_STATUS listen_status = listen(config.server_fd, SOCK_QUEUE_LIMIT);
	if (listen_status == SOCK_ERR) logfatal_exit("Unable to start listening for connections.\n");
}

void init_threads() {
	pthread_mutex_init(&stats_mutex, NULL);
}
