#pragma once
#include "types.h"
#include <sys/stat.h>
// src/file_handling.c

char* parse_file_extension(char* filename);
char* read_file(const char* filename, size_t file_size);

int syscall_file_size(const char* filename);
