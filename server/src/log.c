
#include "logger.h"
#include <limits.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>

// tries to show nonprintable characters and escape escape sequences

#define LOWER_SEPARATOR_CH "▔"
#define UPPER_SEPARATOR_CH "▁"

#define LOG_SYMBOL_REPL(symbol, repl)                                          \
    do {                                                                       \
        log_trace("(" UNDERLINE BOLD symbol FMT_CLEAR                          \
                  ")='" PURPLE repl FMT_CLEAR "', ");                          \
    } while (0);

bool show_pretty_print_guide = true;

ssize_t GET_TERM_COLS() {
    struct winsize w;
    ioctl(0, TIOCGWINSZ, &w);
    return (ssize_t)w.ws_col;
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

void dprintbuf(const char *title, const char *buf, ssize_t sz, ssize_t nlines) {
    log_trace(UNDERLINE BOLD);
    log_trace(UNDERLINE BOLD "%s: ", title);
    if (nlines > 0) {
        log_trace("PRINTING FIRST %zu LINES:%s\n", nlines, FMT_CLEAR);
    } else {
        log_trace(FMT_CLEAR "\n");
        nlines = LONG_MAX;
    }

    LOG_UPPER_SEPARATOR();
    ssize_t lc = 0;
    bool queue_newline = true;
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
            log_trace(UNDERLINE BOLD " " FMT_CLEAR);
            break;

        // only CRLF should print newline
        case '\n':
            log_trace(UNDERLINE BOLD "↓" FMT_CLEAR);
            queue_newline = (DBPRINTBUF_CRLF_ONLY) ? (prevch == '\r') : (true);
            break;
        case '\r':
            log_trace(UNDERLINE BOLD "↵" FMT_CLEAR);
            break;
        default:
            log_trace("%c", ch);
            break;
        }
        if (nextch == '\0')
            log_trace(UNDERLINE BOLD "[\\0]" FMT_CLEAR);
    }

    if (lc <= nlines)
        log_trace("\n");
    LOG_LOWER_SEPARATOR();
    log_trace("\n");
    if (show_pretty_print_guide) {
        log_trace(FMT_CLEAR);
        LOG_SYMBOL_REPL(" ", "SPACE");
        LOG_SYMBOL_REPL("↓", "\\n");
        LOG_SYMBOL_REPL("↵", "\\r");
        log_trace("\n");
        show_pretty_print_guide = false;
    }
}
