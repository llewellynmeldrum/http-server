#include <stdio.h>
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
int handle_client(void* arg);

typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;
typedef int FILE_DESCRIPTOR;

FILE_DESCRIPTOR server;
unsigned short PORT = 49931;


void handle_SIGINT();

// *INDENT-OFF*
int main(int argc, char** argv) {
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
	while(accepting_connections){
		// 1. init client file descriptor
		sockaddr_in client_addr;
		socklen_t client_addr_len = sizeof(client_addr);
		FILE_DESCRIPTOR * client= malloc(sizeof(FILE_DESCRIPTOR)); // 2. listen for client connections - accept blocks the thread until it finds one 
		log("Listening on port %hu...\n", PORT); 
      		log("Avaliable at: http://127.0.0.1:%hu\n",PORT);
		*client = accept(server, (sockaddr*)&client_addr, &client_addr_len);
		if (*client==SOCK_ERR){
			logfatalerrno("Unable to accept client connection:\n");
			free(client);	
			logexit(EXIT_FAILURE);
		}

		// 3. handle the connection
		if (handle_client(client) == EXIT_FAILURE){
			logexit(EXIT_FAILURE);
		}
	}

	logexit(EXIT_SUCCESS);
}

int handle_client(void* arg){
	int RETVAL = EXIT_SUCCESS;

	char* buf = malloc(BUF_SZ);
	FILE_DESCRIPTOR client = *(int*)arg;
	ssize_t bytes_received = recv(client, buf, BUF_SZ, 0);
	if (bytes_received<=0){
		RETVAL = EXIT_FAILURE;
		logwarning("Connection made with client, but <=0 bytes (%zu) receieved.\n",bytes_received);
		goto CLIENT_CLEANUP;
	}
	log("Recieved %zu bytes from client.\n",bytes_received);
	log("%s",buf);
	dprintbuf("RECEIVED MESSAGE", buf, bytes_received, 0);

	char* request_path= malloc(sizeof(char) * BUF_SZ); // this doesnt need dynamic memory allocation
	if (!request_path){
		logfatal("Unable to alloc buffer for request_path!\n");
		RETVAL = EXIT_FAILURE;
		goto CLIENT_CLEANUP;
	}

	int count = sscanf(buf, "GET %s HTTP/1.1", request_path);
	if (count!=1){
		logfatal("Message above has poorly formatted header.\n")
		RETVAL = EXIT_FAILURE;
		goto CLIENT_CLEANUP;
	}

	log("\nrequest path string --> '%s'\n",request_path);
	char* file_name = request_path; // remove %20 spaces and shit later
	char* response = build_http_response(file_name);
	free(request_path);
	dprintbuf("SENDING MESSAGE", response, strlen(response), 0);
	// make 0 lc print them all and not show that stupid 2nd part of the message

	log("\nsending response...\n%s",response);

	ssize_t bytes_sent = send(client, response, strlen(response), 0);
	if (bytes_sent==SOCK_ERR){
		logfatalerrno("Failed to send above http response!!\n");
	} else {
		log("Succesfully sent (%zu) bytes in response!\n",bytes_sent);
	}
	free(response);

CLIENT_CLEANUP:
	close(client);
	free(buf);
	return RETVAL;
}

void handle_SIGINT() {
	log("<-- handling SIGINT.\n");
	int opt = 1;
	setsockopt(server, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
	logexit(EXIT_SUCCESS);
}
