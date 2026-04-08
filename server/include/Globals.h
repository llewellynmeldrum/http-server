#pragma once
#include <stdbool.h>
#include <stddef.h>
static const int MAX_LISTENER_THREADS = 10;  // TODO: thread pool instead?

// TODO: fix names and usage
static constexpr size_t BUF_SZ = 1024;
static constexpr size_t MAX_HEADER_COUNT = 128;
static constexpr size_t MAX_PATH_SEGMENTS = 64;
static const size_t     MAX_METHOD_STRLEN = 32;
static const size_t     MAX_TARGET_STRLEN = 256;
static const size_t     MAX_FILE_SZ = (1e9);  // 1Gb ~(125MiB)
// static const size_t     MAX_RESPONSE_HEADER_SZ = 1024;
