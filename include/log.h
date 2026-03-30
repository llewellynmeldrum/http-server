#ifndef LOG_H
#define LOG_H

#include "errno_helper.h"

#include <errno.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>

extern bool show_pretty_print_guide;
extern char copybuf[];

ssize_t GET_TERM_COLS();
void LOG_UPPER_SEPARATOR();
void LOG_LOWER_SEPARATOR();
void dprintbuf(const char *title, const char *buf, ssize_t sz,
               ssize_t linec_to_print);
#define pretty_print_buffer(title, buf, lncount)                               \
    dprintbuf(title, buf, strlen(buf), lncount);

#define log(fmt, ...) fprintf(stderr, fmt, ##__VA_ARGS__)

#define logexit(n)                                                             \
    do {                                                                       \
        if (n == EXIT_SUCCESS) {                                               \
            log(SET_GREEN);                                                    \
        } else {                                                               \
            log(SET_RED);                                                      \
        }                                                                      \
        log("\nExiting...%s\n\n", SET_CLEAR);                                  \
        exit(n);                                                               \
    } while (0)

#define logerrno() _logerrno(__LINE__, __FILE_NAME__)
inline static void _logerrno(int line, const char *filename) {
    log("%s:%d -> ERROR", filename, line);
    log(": %s (%d), %s:", errno_id_str[errno], errno, errno_meaning_str[errno]);
}

/* functions can define FN_NAME to idenify themselves in logging:
 * 	- any FATAL messages,
 *   	- any WARNING messages (planned)
 *   	- any CRIT_WARNING messages (planned)
 * */

#define ALLOC_FAILURE(resource)                                                \
    "Failed to allocate memory for '%s'!\n", #resource
#ifdef FN_NAME

#define logfatal(fmt, ...)                                                     \
    do {                                                                       \
        log("\n\033[31;1m[FATAL ERROR IN " FN_NAME "] ->\033[0m \033[31m");    \
        log(fmt, ##__VA_ARGS__);                                               \
        log("\033[0m");                                                        \
    } while (0)

#else
#define logfatal(fmt, ...)                                                     \
    do {                                                                       \
        log("\n\033[31;1m[FATAL ERROR] ->\033[0m \033[31m");                   \
        log(fmt, ##__VA_ARGS__);                                               \
        log("\033[0m");                                                        \
    } while (0)

#endif

#define logfatalerrno(fmt, ...)                                                \
    do {                                                                       \
        logfatal(fmt, ##__VA_ARGS__);                                          \
        logerrno();                                                            \
        log("\033[0m");                                                        \
    } while (0)

#define logfatal_exit(fmt, ...)                                                \
    do {                                                                       \
        logfatal(fmt, ##__VA_ARGS__);                                          \
        logexit(1);                                                            \
    } while (0)

#define logfatalerrno_exit(fmt, ...)                                           \
    do {                                                                       \
        logfatalerrno(fmt, ##__VA_ARGS__);                                     \
        logexit(1);                                                            \
        exit(1);                                                               \
    } while (0)

#define logwarning(fmt, ...)                                                   \
    do {                                                                       \
        log(SET_ORANGE);                                                       \
        log("\n[WARNING!]%s\n", SET_CLEAR);                                    \
        log(SET_BOLD);                                                         \
        log(fmt, ##__VA_ARGS__);                                               \
    } while (0)

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

#define SET(s)                                                                 \
    "\033["s                                                                   \
    "m"

#endif // LOG_H
