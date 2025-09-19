/* should only ever be included once, currently by main.c!!*/

#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

typedef int SOCK_STATUS;


static const SOCK_STATUS SOCK_ERR = -1;




/* THREADING STUFF */
static pthread_t client_handler_thread;
static pthread_mutex_t stats_mutex;

struct ServerStats {
	ssize_t requests_received;
	ssize_t bytes_received;

	ssize_t results_sent;
	ssize_t bytes_sent;
};
typedef struct ServerStats ServerStats;



#define STR_USAGE "[-l]\n"	\
"	-l/--local	Specify that you are hosting the server locally. (optional)"

#define FSTR_AVALIABLE_AT(config) \
"Listening on port %hu...\n\
Avaliable at: http://%s:%hu\n", config.port, config.ipv4_str, config.port

#define FSTR_STATS(stats) \
"%.30s\t%zu/%zu\n%.30s\t%zu/%zu\n", \
"Requests sent/received",  stats.results_sent, stats.requests_received, \
"Bytes sent/received", stats.bytes_sent, stats.bytes_received
