#include "str_extras.h"
// true iff the first `strlen(prefix)` chars of `str` exactly match `prefix`
#include <stdbool.h>
#include <string.h>
bool cstr_startsWith(const char *str, const char *prefix) {
    if (!str || !prefix) {
        return false;
    }

    for (int i = 0; i < strlen(prefix); i++) {
        if (str[i] != prefix[i]) {
            return false;
        }
    }
    return true;
}
