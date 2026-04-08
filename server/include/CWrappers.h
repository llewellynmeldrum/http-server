#pragma once
#include "Debug.h"
#include "Logger.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

// #define ENABLE_MEMCPY_DEBUG
static int  ___global = 0;
static char memcpy_hint[BUF_SZ] = "";

static inline char* ptr_details(const void* ptr, int count, const char* hint) {
    int           stack = 0;
    constexpr i64 GUESS_TOLERANCE = 32768;

    const i64 stack_addr = (i64)(&stack);
    const i64 global_addr = (i64)(&___global);

    i64 dist_from_stack = llabs((i64)ptr - stack_addr);

    i64 dist_from_global = llabs((i64)ptr - global_addr);

    // LOG_EXPR(dist_from_stack);
    // LOG_EXPR(dist_from_global);
    char guessed_location[BUF_SZ] = {};
    if (dist_from_stack < GUESS_TOLERANCE) {
        snprintf(guessed_location, BUF_SZ, "%s", "ST");  // stack
    } else if (dist_from_global < GUESS_TOLERANCE) {
        snprintf(guessed_location, BUF_SZ, "%s", "GL");  // global
    } else {
        snprintf(guessed_location, BUF_SZ, "%s", "HE");  // heap
    }

    char buf[BUF_SZ] = {};
    if (hint && streq(hint, "cstr")) {
        snprintf(buf, BUF_SZ, "(\"%.*s\", %s)", count, (char*)ptr, guessed_location);
    } else {
        snprintf(buf, BUF_SZ, "(%p, %s)", ptr, guessed_location);
    }
    char* res = calloc(strlen(buf) + 1, sizeof(char));
    strcpy(res, buf);
    return res;
}
#define SET_MEMCPY_HINT(hint) strncpy(memcpy_hint, #hint, BUF_SZ);
#ifdef ENABLE_MEMCPY_DEBUG
#undef memcpy
static inline void logged_memcpy(void* dest, const void* src, size_t count, const char* filename,
                                 size_t line, const char* hint) {
    char* src_details = ptr_details(src, count, hint);
    char* dest_details = ptr_details(dest, count, hint);
    log_internal(LogLevel_DEBUG, filename, line, "copying %zuB from: %s to %s. ", count,
                 src_details, dest_details);
    memcpy(dest, src, count);
    free(src_details);
    free(dest_details);
}
#define memcpy(dest, src, count)                                                                   \
    logged_memcpy(dest, src, count, __FILE_NAME__, __LINE__, memcpy_hint)
// the classic. Some more sophisticated folk might like the __builtin_types_compatible_p
// variant, which ensures this is not used on pointers, but my setup of clangd yells at me enough.
#endif
