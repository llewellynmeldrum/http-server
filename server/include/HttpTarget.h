#pragma once
#include "HttpRequest.h"
#include "LString.h"
#include <stdio.h>
struct HttpTarget {
    String    resolved_path;
    bool      hasError;
    HttpError err;
};
typedef struct HttpTarget HttpTarget;

void       init_percentEncodingMap(void);
HttpTarget resolve_HttpRequest(HttpRequest request);
