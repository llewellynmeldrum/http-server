
#include <limits.h>
#include <sys/ioctl.h>
#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include "log.h"

// tries to show nonprintable characters and escape escape sequences

#define LOWER_SEPARATOR_CH "▔"
#define UPPER_SEPARATOR_CH "▁"

#define LOG_SYMBOL_REPL(symbol, repl)do{\
	log("(" SET_UNDERBOLD symbol SET_CLEAR ")='" SET_PURPLE repl SET_CLEAR "', ");\
	}while(0);

bool ppbuffer_called = false;

ssize_t GET_TERM_COLS() {
	struct winsize w;
	ioctl(0, TIOCGWINSZ, &w);
	return (ssize_t)w.ws_col;
}
void LOG_UPPER_SEPARATOR() {
	for (int i = 0; i < GET_TERM_COLS(); i++)
		log("%s", UPPER_SEPARATOR_CH);
	log("\r");
}
void LOG_LOWER_SEPARATOR() {
	for (int i = 0; i < GET_TERM_COLS(); i++)
		log("%s", LOWER_SEPARATOR_CH);
	log("\r");
}

/* Whether or not to print a \n IFF CRLF (windows style), or to ignore CR.*/
#define DBPRINTBUF_CRLF_ONLY true

void dprintbuf(const char* title, const char* buf, ssize_t sz, ssize_t nlines) {
	log(SET_UNDERBOLD);
	log(SET_UNDERBOLD "%s: ", title);
	if (nlines > 0) {
		log("PRINTING FIRST %zu LINES:%s\n", nlines, SET_CLEAR);
	} else {
		log(SET_CLEAR "\n");
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
			if (++lc > nlines) break;

			log(SET_BOLD SET_REV "\n%02zu:%s ", lc, SET_CLEAR);
		}

		switch (ch) {
		case ' ':
			log(SET_UNDERBOLD " " SET_CLEAR );
			break;

		// only CRLF should print newline
		case '\n':
			log(SET_UNDERBOLD "↓" SET_CLEAR );
			queue_newline = (DBPRINTBUF_CRLF_ONLY ) ? (prevch == '\r') : (true);
			break;
		case '\r':
			log(SET_UNDERBOLD "↵" SET_CLEAR );
			break;
		default:
			log("%c", ch);
			break;
		}
	}


	if (lc <= nlines) log("\n");
	LOG_LOWER_SEPARATOR();
	log("\n");
	if (!ppbuffer_called) {
		log(SET_CLEAR);
		LOG_SYMBOL_REPL(" ", "SPACE");
		LOG_SYMBOL_REPL("↓", "\\n");
		LOG_SYMBOL_REPL("↵", "\\r");
		log("\n");
		ppbuffer_called = true;
	}
}
void LOG_DEBUG(const char* fmt, ...) {
	/* within fmt, just go char by char and replace any and all '\' with '\\'*/

	char buf[256];
	va_list args;
	va_start(args, fmt);
	vsprintf(buf, fmt, args);
	va_end(args);
	char ch = '1';
	for (int i = 0; ch != '\0'; i++) {
		ch = *(buf + i);
		switch (ch) {
		case '\n':
			log("␊\n");
			break;
		default:
			log("%c", ch);
		}
	}
// TEMP
	log("strlen(log)=%zu", strlen(buf));
}
