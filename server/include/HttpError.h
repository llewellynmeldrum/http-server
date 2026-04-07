#pragma once
#include "HttpMetadata.h"
#include <stddef.h>
typedef struct {
    const char* name;
    size_t      line;
} ErrorSource;

typedef struct {
    HttpStatus  code;
    ErrorSource src;
    const char* info;  // halloced by the error setter, expects to be freed by ??
} HttpError;
