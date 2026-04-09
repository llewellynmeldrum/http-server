#include "HttpParsing.h"
static HttpRequestMethod match_HttpRequestMethod(const StringView sv);
static HttpVersion       match_HttpVersion(const StringView sv);
// RFC 9112:
/*
    OWS := (SP|HTAB)*

    HTTP-message   = start-line CRLF
                    (field-line CRLF)*
                    CRLF
                    [message-body]
*/
HttpRequest parse_HttpRequest(ByteStream* stream) {
    //    bs_debugLog(stream);
    HttpRequest res = {};

    // 1. parse request line
    // RFC 9112:
    //  request-line   = method SP request-target SP HTTP-version CRLF
    StringView method_sv = bs_consumeUntil(stream, SP);
    if (method_sv.len == 0 || !bs_skipExactly(stream, SP)) {
        goto ERR_400_BAD_REQUEST;
    } else if (method_sv.len > MAX_METHOD_STRLEN) {
        goto ERR_501_NOT_IMPLEMENTED;
    }

    StringView target_sv = bs_consumeUntil(stream, SP);
    if (target_sv.len == 0 || !bs_skipExactly(stream, SP)) {
        goto ERR_400_BAD_REQUEST;
    } else if (target_sv.len > MAX_TARGET_STRLEN) {
        goto ERR_414_URI_TOO_LONG;
    }

    StringView version_sv = bs_consumeUntil(stream, CRLF);
    if (version_sv.len == 0 || !bs_skipExactly(stream, CRLF)) {
        goto ERR_400_BAD_REQUEST;
    }

    // 2. Parse field lines
    // RFC 9112:
    // field-line   = field-name ":" OWS field-value OWS
    // OWS = optional whitespace, i.e 0 or more SP|HTAB

    HttpHeader headers[MAX_HEADER_COUNT] = {};
    while (res.num_headers < MAX_HEADER_COUNT) {
        if (sv_equal(bs_lookahead(stream, 2), CRLF)) {
            break;
        }
        StringView field_name = bs_consumeUntil(stream, COLON);
        if (field_name.len == 0 || !bs_skipExactly(stream, COLON)) {
            goto ERR_400_BAD_REQUEST;
        }
        StringView field_value = bs_consumeUntil(stream, CRLF);
        field_value = sv_strip(field_value, OWS);
        //        sv_print(field_value);
        headers[res.num_headers++] = requestHeader_make(field_name, field_value);
        (void)bs_consumeN(stream, 2);  // skip 'CRLF'
    }
    if (res.num_headers >= 1) {
        res.headers = calloc(res.num_headers, sizeof(HttpHeader));
        memcpy(res.headers, headers, res.num_headers * sizeof(HttpHeader));
    } else {
        res.headers = nullptr;
    }
    res.method = match_HttpRequestMethod(method_sv);
    if (res.method == HttpRequestMethod_BAD_REQUEST) {
        goto ERR_400_BAD_REQUEST;
    } else if (res.method >= HttpRequestMethod_OPTIONS) {
        goto ERR_501_NOT_IMPLEMENTED;
    }
    res.target_sv = target_sv;
    res.version = match_HttpVersion(version_sv);
    if (res.version != HttpVersion_1_1) {
        goto ERR_505_HTTP_VERSION_NOT_SUPPORTED;
    }
    LOG_DEBUG("Parsed http request:");
    // LOG_EXPR(res);
    return res;

// [RFC 9110]:
// *'A server can send a 505 (HTTP Version Not Supported) response if it wishes, for any reason, to
// refuse service of the client's major protocol version'*
ERR_505_HTTP_VERSION_NOT_SUPPORTED:
    SET_ERROR(res, HTTP_VERSION_NOT_SUPPORTED);
    return res;

// [RFC 9112]:
// *'When a [HTTP] server (...) receives a sequence of octets that does not match
// the HTTP-message grammar (...) the server SHOULD respond with a 400'*
ERR_400_BAD_REQUEST:
    SET_ERROR(res, BAD_REQUEST);
    return res;

// [RFC 9112]:
// *'A server that receives a request-target longer than any URI it wishes to parse MUST
// respond with a 414'*
ERR_414_URI_TOO_LONG:
    SET_ERROR(res, URI_TOO_LONG);
    return res;

// [RFC 9112]:
// *'A server that receives a method longer than any that it implements
// SHOULD respond with a 501'*
// [RFC 9110]:
// *'The 501 (Not Implemented) status code indicates that the server does not support the
// functionality required to fulfill the request. This is the appropriate response when the server
// does not recognize the request method and is not capable of supporting it for any resource'*
ERR_501_NOT_IMPLEMENTED:
    SET_ERROR(res, NOT_IMPLEMENTED);
    return res;
}
static HttpRequestMethod match_HttpRequestMethod(const StringView sv) {
    for (HttpRequestMethod i = 0; i < HttpRequestMethod__COUNT; i++) {
        const char* method_str = HttpRequestMethod_toStr[i];
        if (sv_matchesStr(sv, method_str)) {
            return i;
        }
    }
    return HttpRequestMethod_BAD_REQUEST;
}

static HttpVersion match_HttpVersion(const StringView sv) {
    for (HttpVersion i = 0; i < HttpVersion__COUNT; i++) {
        const char* version_str = HttpVersion_toStr[i];
        if (sv_matchesStr(sv, version_str)) {
            return i;
        }
    }
    return HttpVersion_NOT_SUPPORTED;
}
