#include <stdio.h>

#include "file_handling.h"
#include "http_response_internal.h"
#include "logger.h"

int syscall_file_size(const char* filename) {
    struct stat s;
    stat(filename, &s);
    return s.st_size;
}

// returns a byte array
char* read_file(const char* filename, size_t file_size) {
    FILE* file_ptr = fopen(filename, "rb");
    char* file_contents = nullptr;

    if (!file_ptr) {
        LOG_FATAL("Unable to open file '%s':", filename);
        LOG_ERRNO();
        return nullptr;
    }

    file_contents = malloc(sizeof(char) * file_size);

    if (!file_contents) {
        LOG_FATAL("Unable to alloc buffer for file '%s'.\n", filename);
        goto read_file_CLEANUP;
    }

    int bytes_read = fread(file_contents, file_size, 1, file_ptr);
    if (bytes_read != 1) {
        LOG_FATAL("Unable to read file contents for file '%s'.\n", filename);
        goto read_file_CLEANUP;
    }
read_file_CLEANUP:
    fclose(file_ptr);
    return file_contents;
}

char* parse_file_extension(char* filename) {
    if (!filename) {
        LOG_FATAL("Recieved null filename.");
        return nullptr;
    }
    char* last_dot = strrchr(filename, '.');
    if (!last_dot || last_dot == filename || strrchr(last_dot, '/')) {
        return nullptr;
    }
    return last_dot;
}
