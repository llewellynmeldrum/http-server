#include "Logger.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

// tries to show nonprintable characters and escape escape sequences

#define LOWER_SEPARATOR_CH "▔"
#define UPPER_SEPARATOR_CH "▁"

void logSymbolReplacement(const char* symbol, const char* replacement) {
    log_trace("(%s%s%s)", UNDERBOLD, symbol, FMT_CLEAR);
    log_trace("=%s%s%s,", PURPLE, replacement, FMT_CLEAR);
}

bool show_pretty_print_guide = true;

size_t GET_TERM_COLS() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return (size_t)w.ws_col;
}
void LOG_UPPER_SEPARATOR() {
    for (int i = 0; i < GET_TERM_COLS(); i++)
        log_trace("%s", UPPER_SEPARATOR_CH);
    log_trace("\r");
}
void LOG_LOWER_SEPARATOR() {
    for (int i = 0; i < GET_TERM_COLS(); i++)
        log_trace("%s", LOWER_SEPARATOR_CH);
    log_trace("\r");
}

/* Whether or not to print a \n IFF CRLF (windows style), or to ignore CR.*/
#define DBPRINTBUF_CRLF_ONLY true

// printLongBuffer(const char* buf, size_t sz)
// printBuffer(const char* buf, size_t sz) - single line
// print a buffer with info on nonprintable chars
void printBuffer(const char* buf, size_t sz) {
}
void print_buffer_verbose(const char* title, const char* buf, size_t sz, size_t nlines) {
    log_trace(UNDERBOLD);
    log_trace(UNDERBOLD "%s: (%p)", title, title);
    if (nlines > 0) {
        log_trace("PRINTING FIRST %zu LINES:%s\n", nlines, FMT_CLEAR);
    } else {
        log_trace(FMT_CLEAR "\n");
        nlines = LONG_MAX;
    }

    LOG_UPPER_SEPARATOR();
    size_t lc = 0;
    bool   queue_newline = true;
    for (int i = 0; i < sz; i++) {
        char prevch = (i - 1 >= 0) ? buf[i - 1] : '\0';
        char ch = buf[i];
        char nextch = (i + 1 < sz) ? buf[i + 1] : '\0';

        if (queue_newline) {
            queue_newline = false;
            if (++lc > nlines)
                break;

            log_trace(BOLD REV "\n%02zu:%s ", lc, FMT_CLEAR);
        }

        switch (ch) {
        case ' ':
            log_trace(UNDERBOLD " " FMT_CLEAR);
            break;

        // only CRLF should print newline
        case '\n':
            log_trace(UNDERBOLD "↓" FMT_CLEAR);
            queue_newline = (DBPRINTBUF_CRLF_ONLY) ? (prevch == '\r') : (true);
            break;
        case '\r':
            log_trace(UNDERBOLD "↵" FMT_CLEAR);
            break;
        default:
            log_trace("%c", ch);
            break;
        }
        if (nextch == '\0')
            log_trace(UNDERBOLD "[\\0]" FMT_CLEAR);
    }

    if (lc <= nlines) {
        log_trace("\n");
    }
    LOG_LOWER_SEPARATOR();
    log_trace("\n");
    if (show_pretty_print_guide) {
        log_trace(FMT_CLEAR);
        logSymbolReplacement(" ", "SPACE");
        logSymbolReplacement("↓", "\\n");
        logSymbolReplacement("↵", "\\r");
        log_trace("\n");
        show_pretty_print_guide = false;
    }
}
