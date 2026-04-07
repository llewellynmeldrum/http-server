#pragma once

#include <ctype.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "AnsiColors.h"
#include "ErrnoDefs.h"
#include "HttpMetadata.h"
#include "HttpRequest.h"
#include "NativeTimer.h"
#include "cstrHelpers.h"

#define OSTREAM stdout

static inline void log_trace(const char* fmt, ...) {
    va_list ap;
    va_start(ap, fmt);
    vfprintf(OSTREAM, fmt, ap);
    va_end(ap);
}
static inline void LOG_ERRNO(void) {
    log_trace("\t%s(%d) = '%s'\n", errno_id_str[errno], errno, errno_meaning_str[errno]);
}

static inline void log_return_internal(const char* func_str, const char* expr_str) {
    double ms = ms_since_start();
    log_trace(FMT_CLEAR);
    log_trace("%06.3lfs ", ms / 1000.0);
    log_trace(CYAN);
    log_trace("%-8s ", "FUNCTION");
    log_trace(FMT_CLEAR);

    log_trace(BOLD_DWHITE);
    log_trace("%s: ", func_str);
    log_trace(FMT_CLEAR);
    log_trace("returned -> %s", expr_str);
    log_trace("\n");
    if (!streq(expr_str, "void.")) {
        free((void*)expr_str);
    }
}

#define RETURN_ARGC0() return
#define RETURN_ARGC01(expr) return expr

#ifdef DEBUG_RETURNS
#define RETURN(...)                                                                                \
    MACRO_BEGIN                                                                                    \
    LOG_RETURN_ARGC0##__VA_OPT__(1)(__VA_ARGS__);                                                  \
    RETURN_ARGC0##__VA_OPT__(1)(__VA_ARGS__);                                                      \
    MACRO_END

#define LOG_RETURN_ARGC0() log_return_internal(__FUNCTION__, "void.")

#define LOG_RETURN_ARGC01(expr) log_return_internal(__FUNCTION__, TOSTR(expr))

#else
#define RETURN(...) RETURN_ARGC0##__VA_OPT__(1)(__VA_ARGS__)
#endif

static const char* FMT_LOGLEVEL_COLORS[] = {
    CYAN,    // DEBUG
    CYAN,    // RETURN
    LIGREY,  // INFO
    YELLOW,  // NOTICE
    PINK,    // WARN
    LIRED,   // ERROR
    RED,     // FATAL
    GREEN,   // EXIT_SUCCESS
    RED,     // EXIT_FAILURE
};
// clang-format off
static const char* loglevel_tostr[] = { 
    "[DEBUG]", 
    "[DEBUG-RET]", 
    " [INFO]",
    "[NOTIF]",
    " [WARN]",
    "[ERROR]",
    "[FATAL]",
    "  [N/A]",
    "[EXIT]",
    "[EXIT]", 
};

typedef enum LogLevel {
    LogLevel_DEBUG,
    LogLevel_DEBUG_RETURN,
    LogLevel_INFO,
    LogLevel_NOTICE,
    LogLevel_WARN,
    LogLevel_ERROR,
    LogLevel_FATAL,
    LogLevel__COUNT,
    LogLevel_EXIT_SUCCESS,
    LogLevel_EXIT_FAILURE,
} LogLevel;

// everything below this will be ignored
#define LOGLEVEL LogLevel_DEBUG
static inline void log_internal(LogLevel level, const char* filename, int line, const char* fmt, ...) {
    if (level<LOGLEVEL){
        return;
    }
    #ifdef LOGGER_DISABLE_TIMER
    double ms = ms_since_start();
    log_trace(FMT_CLEAR);
    log_trace("%06.3lfs ", ms / 1000.0);
    #endif
    log_trace(FMT_LOGLEVEL_COLORS[level]);
    log_trace("%-8s", loglevel_tostr[level]);
    if (level!=LogLevel_FATAL){
        log_trace(FMT_CLEAR);
        log_trace(BOLD_DWHITE);
    } else{
        log_trace(BOLD);
    }
    log_trace("%s:%d: ", filename, line);
    if (!(level==LogLevel_FATAL)){
        log_trace(FMT_CLEAR);
    }
    va_list ap;
    va_start(ap, fmt);
    vfprintf(OSTREAM, fmt, ap);
    va_end(ap);
    fprintf(OSTREAM, "\n");
        log_trace(FMT_CLEAR);
    if (level==LogLevel_EXIT_FAILURE || level==LogLevel_EXIT_SUCCESS){
        exit(level);
    }
}

typedef struct LogSettings {
    bool showFunctions;
} LogSettings;
static LogSettings log_settings;
#define SETLOG_SHOWFUNCTIONS(val) log_settings.showFunctions = val;
// clang-format off
#define LOG_DEBUG(fmt, ...)         log_internal(LogLevel_DEBUG,   __FILE_NAME__, __LINE__, fmt,  ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)          log_internal(LogLevel_INFO,    __FILE_NAME__, __LINE__, fmt,   ##__VA_ARGS__)
#define LOG_NOTICE(fmt, ...)        log_internal(LogLevel_NOTICE,  __FILE_NAME__, __LINE__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)          log_internal(LogLevel_WARN,    __FILE_NAME__, __LINE__, fmt,   ##__VA_ARGS__)
#define LOG_ERROR(fmt, ...)         log_internal(LogLevel_ERROR,   __FILE_NAME__, __LINE__, fmt,  ##__VA_ARGS__)
#define LOG_FATAL(fmt, ...)         log_internal(LogLevel_FATAL,    __FILE_NAME__, __LINE__, fmt,  ##__VA_ARGS__)
#define LOG_EXIT(code)              log_internal(code+LogLevel__COUNT,             __FILE_NAME__, __LINE__, "Exiting. (Code:%d)",code)


#ifdef __APPLE__
    #define UINT64_T_FMT "%llu"
#elif __linux__
    #define UINT64_T_FMT "%lu"
#else
    #error Unsupported operating system
#endif

// clang-format on
#define X_LIST_TYPENAMES                                                                           \
    X(int, "%d", val)                                                                              \
    X(char, "'%c'", val)                                                                           \
    X(uint64_t, UINT64_T_FMT, val)                                                                 \
    X(long, "%ld", val)                                                                            \
    X(double, "%lf", val)                                                                          \
    X(bool, "%s", (val ? "true" : "false"))                                                        \
    X(HttpRequest,                                                                                 \
      "HttpRequest {\n"                                                                            \
      "    .method     = '%s'\n"                                                                   \
      "    .target_sv  = '%s'\n"                                                                   \
      "    .version    = '%s'\n"                                                                   \
      "    .headers    = %s\n"                                                                     \
      "} HttpRequest;",                                                                            \
      HttpRequestMethod_toStr[val.method], sv_cstr(val.target_sv), HttpVersion_toStr[val.version], \
      svparr_ToStr(val.headers, val.num_headers))

#define X(T, fmt, ...)                                                                             \
    static inline const char* T##_toStr(T val) {                                                   \
        char* buf = calloc(BUF_SZ, sizeof(char));                                                  \
        snprintf(buf, BUF_SZ, "%s ", #T);                                                          \
        snprintf(buf, BUF_SZ, fmt, ##__VA_ARGS__);                                                 \
        return buf;                                                                                \
    }

// clang-format on
X_LIST_TYPENAMES
#undef X

// clang-format off
#define TOSTR(x) \
    _Generic(x, \
        int: int_toStr, \
        char: char_toStr, \
        uint64_t: uint64_t_toStr, \
        long: long_toStr, \
        double: double_toStr, \
        bool: bool_toStr, \
        HttpRequest: HttpRequest_toStr, \
        StringView: sv_cstr, \
        String*: str_cstr, \
        default: int_toStr)(x)
// clang-format on

#define LOG_EXPR(x)                                                                                \
    do {                                                                                           \
        const char* str = TOSTR(x);                                                                \
        log_internal(LogLevel_INFO, __FILE_NAME__, __LINE__, "%s = %s", #x, str);                  \
    } while (0)
