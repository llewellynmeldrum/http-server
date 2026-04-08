#pragma once

#include "ByteStream.h"
#include "HttpRequest.h"
HttpRequest parse_HttpRequest(ByteStream* stream);
