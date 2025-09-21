#ifndef LOG_H
#define LOG_H
/* PROPOSED LOGGING LEVELS:

	FATAL;
	-> PREFIX: (bold, red) "[FATAL/UNRECOVERABLE ERROR] --> %s", body_text
	-> BODY TEXT: (red) "..."
		Something likely unrecoverable has happened to the program.
		If termination isnt instant, it should happen pretty soon, else there will be a crash.
		A function that reports a fatal error will almost certainly return a nullptr.
        <------------------------------------------------------------------------------------------>

	CRIT_WARNING;
	-> PREFIX: (bold, underlined(?), yellow) "[CRITICAL WARNING] --> %s", body_text
	-> BODY TEXT: (yellow) "..."
		A critical warning; i.e something which should stand out when reading logs,
		but is not necessarily more pertinent/threatening than a warning.
        <------------------------------------------------------------------------------------------>

	WARNING;
	-> PREFIX: (yellow) "[WARNING] --> %s", body_text
	-> BODY TEXT: (yellow) "..."
		Something probably bad has happened, but we arent going to return null or do anything crazy.
		Just keep it in mind.
        <------------------------------------------------------------------------------------------>

	STANDARD;
	-> PREFIX: (n/a)
	-> BODY TEXT: (n/a) "..."
		The default logging level, shows up no matter what.
        <------------------------------------------------------------------------------------------>

	DEBUG;
	-> PREFIX: (n/a)
	-> BODY TEXT: (blue!) "..."
		For information superfluous outside of specific debugging contexts, like when
		creating a new feature.
        <------------------------------------------------------------------------------------------>

*/
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <stdarg.h>

extern bool show_pretty_print_guide;
extern char copybuf[];

ssize_t GET_TERM_COLS();
void LOG_UPPER_SEPARATOR();
void LOG_LOWER_SEPARATOR();
void dprintbuf(const char* title, const char* buf, ssize_t sz, ssize_t linec_to_print);
#define pretty_print_buffer(title, buf, lncount) dprintbuf(title, buf, strlen(buf), lncount);


#define log(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define logexit(n)do {				\
	if(n == EXIT_SUCCESS){ log(SET_GREEN);}	\
	else {log(SET_RED);}			\
	log("\nExiting...%s\n\n",SET_CLEAR);	\
	exit(n);				\
	} while(0)


#define logerrno() do{			\
	log("ERRNO(%d):",errno);	\
	perror("");			\
	}while(0)


/* functions can define FN_NAME to idenify themselves in logging:
 * 	- any FATAL messages,
 *   	- any WARNING messages (planned)
 *   	- any CRIT_WARNING messages (planned)
 * */

#define ALLOC_FAILURE(resource) "Failed to allocate memory for '%s'!\n", #resource
#ifdef FN_NAME

#define logfatal(fmt, ...) do{							\
	log("\n\033[31;1m[FATAL ERROR IN " FN_NAME "] ->\033[0m \033[31m");	\
	log(fmt, ##__VA_ARGS__);						\
	log("\033[0m");								\
	}while(0)

#else
#define logfatal(fmt, ...) do{					\
	log("\n\033[31;1m[FATAL ERROR] ->\033[0m \033[31m");	\
	log(fmt, ##__VA_ARGS__);				\
	log("\033[0m");						\
	}while(0)

#endif

#define logfatalerrno(fmt, ...) do{	\
	logfatal(fmt, ##__VA_ARGS__);	\
	logerrno();			\
	log("\033[0m");			\
	}while(0)			\

#define logfatal_exit(fmt, ...) do{	\
	logfatal(fmt, ##__VA_ARGS__);	\
	logexit(1);			\
	}while(0)

#define logfatalerrno_exit(fmt, ...) do{	\
	logfatalerrno(fmt, ##__VA_ARGS__);	\
	logexit(1);				\
     	exit(1);				\
	}while(0)

#define logwarning(fmt, ...)do {		\
	log(SET_ORANGE);			\
	log("\n[WARNING!]%s\n",SET_CLEAR);	\
	log(SET_BOLD);				\
	log(fmt, ##__VA_ARGS__);		\
	} while(0)




#define SET_CLEAR "\033[0m"

#define SET_RED "\033[1;31m"
#define SET_GREEN "\033[1;32m"
#define SET_BLUE "\033[0;34m"
#define SET_PURPLE "\033[0;35m"
#define SET_ORANGE "\033[48:2:255:165:1m"


#define SET_REV "\033[7m"

#define SET_BOLD "\033[1m"
#define SET_UNDERLINE "\033[4m"
#define SET_UNDERBOLD "\033[1;4m"


#define SET_NOREV "\033[27m"
#define SET_NOUNDERLINE "\033[24m"
#define SET_NOBOLD "\033[21m"

#define SET(s) "\033["s"m"

#endif // LOG_H
