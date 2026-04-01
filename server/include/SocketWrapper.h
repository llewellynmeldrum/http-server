#pragma once
#include <netinet/in.h>
#include <pthread.h>
#include <stdbool.h>
#include <unistd.h>

typedef int              SOCK_STATUS;
static const SOCK_STATUS SOCK_ERR = -1;
static const SOCK_STATUS SOCK_SUCCESS = 0;

static const char* STR_USAGE = "[-l]\n"
                               "	-l/--local	Specify that you are hosting the server locally. "
                               "(optional)";

static const char* FSTR_AVALIABLE_AT = "Listening on port %hu...\n"
                                       "Avaliable at: http://%s:%hu\n";
