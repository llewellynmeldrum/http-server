#ifndef LOG_H
#define LOG_H
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

extern bool ppbuffer_called;
extern char copybuf[];
ssize_t GET_TERM_COLS();

void LOG_UPPER_SEPARATOR();
void LOG_LOWER_SEPARATOR();

#define pretty_print_buffer(title, buf, lnc) dprintbuf(title, buf, strlen(buf), lnc);


void dprintbuf(const char* title, const char* buf, ssize_t sz, ssize_t linec_to_print);

#define perrno() do{\
	log("ERRNO(%d):",errno);\
	perror("");\
	}while(0)\

#define logline(fmt, ...)do{ \
	sprintf(copybuf, fmt, __VA_ARGS__);\
	fprintf(stderr, "%-*s\n", TERM_COLS,copybuf);\
	}while(0)

#define log(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)
#define logdebug(fmt, ...) LOG_DEBUG(fmt, ##__VA_ARGS__)


#define logfatalexit(fmt, ...) do{	\
	log("\n\033[31;1m[FATAL ERROR] ->\033[0m \033[31m");	\
	log(fmt, ##__VA_ARGS__);\
	log("\nExiting...\n\033[0m");\
     	exit(1);		\
	}while(0)		\

#define logfatalerrnoexit(fmt, ...) do{	\
	log("\n\033[31;1m[FATAL ERROR] ->\033[0m \033[31m");	\
	log(fmt, ##__VA_ARGS__);\
	perrno();\
	log("\nExiting...\n\033[0m");\
     	exit(1);		\
	}while(0)		\

#define logfatal(fmt, ...) do{	\
	log("\n\033[31;1m[FATAL ERROR] ->\033[0m \033[31m");	\
	log(fmt, ##__VA_ARGS__);\
	log("\033[0m");		\
	}while(0);		\


#define logfatalerrno(fmt, ...) do{	\
	log("\n\033[31;1m[FATAL ERROR] ->\033[0m \033[31m");	\
	log(fmt, ##__VA_ARGS__);\
	perrno();\
	log("\033[0m");		\
	}while(0);		\


#define SET_CLEAR "\033[0m"


#define SET_RED "\033[1;31m"
#define SET_GREEN "\033[1;32m"
#define SET_BLUE "\033[0;34m"
#define SET_PURPLE "\033[0;35m"
#define SET_ORANGE "\033[48:2:255:165:1m"


#define SET_REV "\033[7m"
#define SET(s) "\033["s"m"

#define SET_BOLD "\033[1m"
#define SET_UNDERLINE "\033[4m"
#define SET_UNDERBOLD "\033[1;4m"


#define SET_NOREV "\033[27m"
#define SET_NOUNDERLINE "\033[24m"
#define SET_NOBOLD "\033[21m"
#define logexit(n)do {\
	if(n==EXIT_SUCCESS){ log(SET_GREEN);}\
	else {log(SET_RED);}\
	log("\nExiting...%s\n\n",SET_CLEAR);\
	exit(n);\
	} while(0)
//
#define logwarning(fmt, ...)do {\
	log(SET_ORANGE);\
	log("\n[WARNING!]%s\n",SET_CLEAR);\
	log(SET_BOLD);\
	log(fmt, ##__VA_ARGS__);\
	} while(0)
#endif // LOG_H
