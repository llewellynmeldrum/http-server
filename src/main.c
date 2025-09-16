#include <stdio.h>
#include <pthread.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "log.h"
#include "http_consts.h"
#include "http_response.h"

#define AUTO_FIND_NEWSOCK true

#define SOCK_ERR -1
void *handle_client(void* arg);

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef int FILE_DESCRIPTOR;

FILE_DESCRIPTOR server;
unsigned short PORT = 49931;

static pthread_mutex_t log_mutex;
static pthread_mutex_t stats_mutex;

typedef struct server_stats {
	ssize_t requests_received;
	ssize_t bytes_received;

	ssize_t results_sent;
	ssize_t bytes_sent;
} server_stats;

server_stats s_stats = (server_stats) { };


void handle_SIGINT();

// *INDENT-OFF*
int main(int argc, char** argv) {
	pthread_mutex_init(&log_mutex, NULL);
	pthread_mutex_init(&stats_mutex, NULL);
	int err;

	// 1. create the socket as a file descriptor
	server = socket(AF_INET, SOCK_STREAM, 0);
	if (server == SOCK_ERR) logfatalexit("Unable to create socket.\n");

	signal(SIGINT, handle_SIGINT);


	// 2. configure the socket to listen on PORT, on ANY interface, and using ipv4. 
	sockaddr_in server_addr = (sockaddr_in) {
		.sin_family 		= AF_INET,		// ipv4
		 .sin_addr.s_addr 	= INADDR_ANY, 		// all interfaces
		  .sin_port		= htons(PORT),		// hostToNetworkShort()
	};
	err = bind(server, (sockaddr * )&server_addr, sizeof(server_addr));
	if (err){
		if (!AUTO_FIND_NEWSOCK) logfatalerrnoexit("Failed to bind socket to port: %hu.\n", PORT);
		unsigned short newport;
		while (err){
			newport = (rand() %(49152-65535)) + 49152;
//			log("Port %hu failed to bind, testing new port:%hu\n",PORT, newport);
			server_addr.sin_port = htons(newport);
			err = bind(server, (sockaddr * )&server_addr, sizeof(server_addr));
		}
		PORT = newport;
//		printf("Suitable port (%hu) found, continuing...\n", PORT);
	}



	// 3. begin listening, using the configuration specified above
	//
	err = listen(server, SOCK_QUEUE_LIMIT);
	if (err) logfatalexit("Unable to start listening for connections.\n");


	// 4. handle incoming connections

	bool accepting_connections = true;
	log("Listening on port %hu...\n", PORT); 
	log("Avaliable at: http://127.0.0.1:%hu\n",PORT);
	pthread_t client_handler_thread;
	while(accepting_connections){
		// 1. init client file descriptor
		sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		FILE_DESCRIPTOR * client= malloc(sizeof(FILE_DESCRIPTOR)); // 2. listen for client connections - accept blocks the thread until it finds one 
		*client = accept(server, (sockaddr*)&client_addr, &client_addr_len);
		if (*client==SOCK_ERR){
			logfatalerrno("Unable to accept client connection:\n");
			free(client);	
			logexit(EXIT_FAILURE);
		}

		pthread_create(&client_handler_thread, NULL, handle_client, (void*)client);
		pthread_detach(client_handler_thread);
	}

	logexit(EXIT_SUCCESS);
}

void* handle_client(void* arg){
	char* buf = malloc(BUF_SZ);
	FILE_DESCRIPTOR client = *(int*)arg; 
	ssize_t bytes_received = recv(client, buf, BUF_SZ, 0);
	if (bytes_received<=0){
		logwarning("Connection made with client, but <=0 bytes (%zu) receieved.\n",bytes_received);
		goto CLIENT_CLEANUP;
	}

	pthread_mutex_lock(&stats_mutex);
		s_stats.bytes_received += bytes_received;
		++s_stats.requests_received;
	pthread_mutex_unlock(&stats_mutex);

	log("Recieved %zu bytes from client.\n",bytes_received);
	log("%s",buf);
	pretty_print_buffer("RECEIVED MESSAGE", buf, 0);

	char* request_path= malloc(sizeof(char) * BUF_SZ); // this doesnt need dynamic memory allocation
	if (!request_path){
		logfatal("Unable to alloc buffer for request_path!\n");
		goto CLIENT_CLEANUP;
	}

	int count = sscanf(buf, "GET %s HTTP/1.1", request_path);
	if (count!=1){
		logfatal("Message above has poorly formatted header.\n")
		goto CLIENT_CLEANUP;
	}

	pretty_print_buffer("\nrequest path string:",request_path, 0);
	char* file_name = request_path; // remove %20 spaces and shit later
	char* response = build_http_response(file_name);
	ssize_t response_len = strlen(response);
	free(request_path);
	pretty_print_buffer("SENDING MESSAGE", response, 0);
	// make 0 lc print them all and not show that stupid 2nd part of the message

	log("\nsending response...\n%s",response);

	ssize_t bytes_sent = send(client, response, strlen(response), 0);
	if (bytes_sent==SOCK_ERR){
		logfatalerrno("Failed to send above http response!!\n");
	} else {
		log("Succesfully sent (%zu) bytes in response!\n",bytes_sent);
		pthread_mutex_lock(&stats_mutex);
			s_stats.bytes_sent += response_len;
			++s_stats.results_sent;
		pthread_mutex_unlock(&stats_mutex);
	}
	free(response);

CLIENT_CLEANUP:
	close(client);
	free(buf);
	log("Listening on port %hu...\n", PORT); 
	log("Avaliable at: http://127.0.0.1:%hu\n",PORT);
	log("%.30s\t%zu/%zu\n","Requests sent/received",  s_stats.results_sent, s_stats.requests_received);
	log("%.30s\t%zu/%zu\n","Bytes sent/received", s_stats.bytes_sent, s_stats.bytes_received);

	return NULL;
}

void handle_SIGINT() {
	log("<-- handling SIGINT.\n");
	int opt = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	logexit(EXIT_SUCCESS);
}
