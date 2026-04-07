#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "ByteStream.h"
#include "Globals.h"
#include "HttpError.h"
#include "HttpMetadata.h"
#include "LString.h"
#include "ServerTypes.h"
#include "SocketWrapper.h"
#include "StringMap.h"
#include "StringView.h"

#include "HttpRequest.h"

pthread_t                 client_handler_thread;
static const unsigned int HTTP_TCP_PORT = 80;

#define LOGGER_DISABLE_TIMER

#include "Logger.h"

#include "CWrappers.h"
static ServerConfig config;

static void handle_SIGINT(int sig);
static void init_config(int argc, char** argv);
static void init_socket();
static void init_responders();

static void* handle_client(void* arg);

u64  program_epoch_ns;
char document_root[PATH_MAX] = {};

static StringMap        percent_encoding_map = {};
const static StringView percent_keymap[] = {
    (StringView){ .ptr = "%20", .len = 3 }, (StringView){ .ptr = "%21", .len = 3 },
    (StringView){ .ptr = "%23", .len = 3 }, (StringView){ .ptr = "%24", .len = 3 },
    (StringView){ .ptr = "%25", .len = 3 }, (StringView){ .ptr = "%26", .len = 3 },
    (StringView){ .ptr = "%27", .len = 3 }, (StringView){ .ptr = "%28", .len = 3 },
    (StringView){ .ptr = "%29", .len = 3 }, (StringView){ .ptr = "%2A", .len = 3 },
    (StringView){ .ptr = "%2B", .len = 3 }, (StringView){ .ptr = "%2C", .len = 3 },
    (StringView){ .ptr = "%2F", .len = 3 }, (StringView){ .ptr = "%3A", .len = 3 },
    (StringView){ .ptr = "%3B", .len = 3 }, (StringView){ .ptr = "%3D", .len = 3 },
    (StringView){ .ptr = "%3F", .len = 3 }, (StringView){ .ptr = "%5B", .len = 3 },
    (StringView){ .ptr = "%5D", .len = 3 }, (StringView){ .ptr = "%40", .len = 3 },

};
const static VAL_T percent_valmap[] = {
    ' ', '!', '#', '$', '%', '&', '\'', '(', ')', '*',
    '+', ',', '/', ':', ';', '=', '?',  '[', ']', '@',

};

int main(int argc, char** argv) {
    signal(SIGINT, handle_SIGINT);
    program_epoch_ns = get_current_ns();
    percent_encoding_map = sm_make();
    sm_mapArrays(&percent_encoding_map, percent_keymap, percent_valmap, 20);
    //    sm_print(&percent_encoding_map);

    handle_client((void*)0);
    exit(EXIT_SUCCESS);

    init_config(argc, argv);
    init_socket();
    init_responders();

    bool accepting_connections = true;
    while (accepting_connections) {
        sockaddr_in client_addr;
        socklen_t   client_addr_len = sizeof(client_addr);
        // BUG: Why is this a heap allocation?
        FileDescriptor* client = malloc(sizeof(FileDescriptor));
        LOG_INFO(FSTR_AVALIABLE_AT, config.port, config.hostname, config.port);
        if (!client) {
            LOG_FATAL("Unable to allocate memory for client FD!");
            LOG_EXIT(EXIT_FAILURE);
        }
        *client = accept(config.server_fd, (sockaddr*)&client_addr, &client_addr_len);
        if (*client == SOCK_ERR) {
            LOG_FATAL("Unable to accept client connection:");
            LOG_EXIT(EXIT_FAILURE);
        }

        // dispatch and detach thread
        pthread_create(&client_handler_thread, nullptr, handle_client, (void*)client);
        pthread_detach(client_handler_thread);
    }

    LOG_EXIT(EXIT_SUCCESS);
}
void init_responders(void) {
    for (int i = 0; i < PATH_MAX; i++) {
        document_root[i] = '\0';
    }
    if (!realpath("../www", document_root)) {
        LOG_FATAL("Realpath failed on doc root. doc_root:%s\n", document_root);
        perror("Errno");
        LOG_EXIT(EXIT_FAILURE);
    }
}

HttpRequestMethod match_HttpRequestMethod(const StringView sv) {
    for (HttpRequestMethod i = 0; i < HttpRequestMethod__COUNT; i++) {
        const char* method_str = HttpRequestMethod_toStr[i];
        if (sv_matchesStr(sv, method_str)) {
            return i;
        }
    }
    return HttpRequestMethod_BAD_REQUEST;
}

HttpVersion match_HttpVersion(const StringView sv) {
    for (HttpVersion i = 0; i < HttpVersion__COUNT; i++) {
        const char* version_str = HttpVersion_toStr[i];
        if (sv_matchesStr(sv, version_str)) {
            return i;
        }
    }
    return HttpVersion_NOT_SUPPORTED;
}
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
    } else if (method_sv.len > MAX_METHOD_LEN) {
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

    StringViewPair headers[MAX_HEADER_COUNT] = {};
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
        sv_print(field_value);
        headers[res.num_headers++] = svp_make(field_name, field_value);
        (void)bs_consumeN(stream, 2);  // skip 'CRLF'
    }
    if (res.num_headers >= 1) {
        res.headers = calloc(res.num_headers, sizeof(StringViewPair));
        memcpy(res.headers, headers, res.num_headers * sizeof(StringViewPair));
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

typedef struct {
    FILE*     fptr;
    String    resolved_path;
    bool      hasError;
    HttpError err;
} HttpResource;

String resolve_absoluteForm(StringView raw_target) {
    // if matches http://lmeldrum.dev ->
    assert(sv_hasPrefixStr(raw_target, "http://"));
    const size_t domain_name_start = 7;  // http://<X>
    StringView   no_scheme = sv_trimLeft(raw_target, domain_name_start);

    const size_t first_slash = sv_find(no_scheme, '/');
    no_scheme = sv_trimRight(raw_target, first_slash);

    if (!sv_matchesStr(no_scheme, "lmeldrum.dev")) {
        return NULL_STRING;
    }
    StringView path = sv_trimLeft(no_scheme, first_slash);  // https://lmeldrum.dev/<X>

    const size_t first_question_mark = sv_find(no_scheme, '?');
    path = sv_trimRight(path, first_question_mark);                 // .../<path>[?query]
    StringView query = sv_trimLeft(path, first_question_mark + 1);  // ...?<query>
    (void)query;

    String res = str_make(path);
    // we now have the path, so resolve it.
    return res;
}

String resolve_originForm(StringView raw_target) {
    const size_t first_question_mark = sv_find(raw_target, '?');

    LOG_EXPR(raw_target);
    LOG_EXPR(first_question_mark);
    StringView path = sv_trimRight(raw_target, first_question_mark);      // .../<path>[?query]
    StringView query = sv_trimLeft(raw_target, first_question_mark + 1);  // ...?<query>
    LOG_EXPR(path);
    LOG_EXPR(query);
    (void)query;
    return str_make(path);
}

HttpResource resolve_HttpResource(String target) {
    // given some normalized path
    return (HttpResource){};
}

String decode_percentEscapes(String path) {
    // TODO: path here could be a stringview.
    const size_t lookahead_dist = 2;
    String       res = str_make_empty();
    for (size_t i = 0; i < path.len; i++) {
        if (i < path.len - lookahead_dist && str_at(&path, i) == '%') {
            StringView slice = str_slice(&path, i, i + lookahead_dist);
            if (sm_contains(&percent_encoding_map, slice)) {
                const char repl = sm_find(&percent_encoding_map, slice);
                str_append_ch(&res, repl);
                i += lookahead_dist;  // skip the chars
                continue;
            }
        } else {
            str_append_ch(&res, str_at(&path, i));
            //            LOG_DEBUG("res:%s", str_cstr(&res));
        }
    }
    LOG_EXPR(&res);
    return res;
}
String decode_upwardPathing(String path) {
    // /hello/world
    // given a string of paths /.../.../...
    // 1. split into /<chunk>/ separated chunks
    size_t      num_segments = 0;
    StringView* path_segments = str_splitOnEach(&path, '/', &num_segments);
    // BUG: questionable buf size
    StringView* stack[BUF_SZ] = {};
    size_t      rsp = 0;
    LOG_EXPR(num_segments);
    for (size_t i = 0; i < num_segments; i++) {
        //        LOG_EXPR(path_segments[i]);
        if (sv_equal(path_segments[i], EMPTY) || sv_equal(path_segments[i], DOT)) {
            continue;
        } else if (sv_equal(path_segments[i], DOTDOT)) {
            if (rsp <= 0) {
                return NULL_STRING;  // stack is empty, tried to escape docroot
            } else {
                rsp--;
                //          LOG_DEBUG("POP:");
                //         LOG_EXPR(path_segments[i]);
            }
        } else {
            //    LOG_DEBUG("PUSH:");
            //   LOG_EXPR(path_segments[i]);
            stack[rsp++] = &path_segments[i];
        }
    }

    String res = str_make_empty();
    for (size_t i = 0; i < rsp; i++) {
        str_append_sv(&res, FSLASH);
        str_append_sv(&res, *stack[i]);
    }
    // 2. for each, push.
    // 3. pop off a dir if '..', if st_empty, return err.
    // possible forms: /    /foo.c
    return res;
}
String normalize_Path(String path) {
    String no_percents = decode_percentEscapes(path);
    LOG_EXPR(&no_percents);
    String no_dots = decode_upwardPathing(no_percents);
    LOG_EXPR(&no_dots);
    if (no_dots.isNull) {
        LOG_FATAL("Path (%s) escapes docroot!", str_cstr(&no_percents));
    }
    if (no_dots.len == 0) {
        str_append_ch(&no_dots, '/');
    }
    if (str_last(&no_dots) == '/') {
        str_append_sv(&no_dots, INDEX_HTML);
    }
    return no_dots;
}

String resolve_HttpTarget(StringView raw_target) {
    //    find out if absolute_form or origin_form;
    typedef enum {
        HttpResourceType_ABSOLUTE,
        HttpResourceType_ORIGIN,
        HttpResourceType_BAD_FORM,
    } HttpResourceType;
    HttpResourceType req_type;
    if (sv_hasPrefixStr(raw_target, "http://")) {
        req_type = HttpResourceType_ABSOLUTE;
    } else if (sv_at(raw_target, 0) == '/') {
        req_type = HttpResourceType_ORIGIN;
    } else {
        req_type = HttpResourceType_BAD_FORM;
    }

    switch (req_type) {
    case HttpResourceType_ABSOLUTE:
        return resolve_absoluteForm(raw_target);
        break;
    case HttpResourceType_ORIGIN:
        return resolve_originForm(raw_target);
        break;
    case HttpResourceType_BAD_FORM:
        return NULL_STRING;
        break;
    }
}
HttpResource resolve_HttpRequest(HttpRequest request) {
    HttpResource res = {};
    if (request.hasError) {
        res.resolved_path = NULL_STRING;
        res.fptr = nullptr;
        res.hasError = true;
        res.err = request.err;
        // pass the error forward
        return res;
    }

    String target = resolve_HttpTarget(request.target_sv);
    if (str_equal(&target, &NULL_STRING)) {
        goto ERR_501_NOT_IMPLEMENTED;
    }
    String normalized_path = normalize_Path(target);
    if (str_equal(&target, &NULL_STRING)) {
    }
    LOG_EXPR(&normalized_path);
    str_prepend_sv(&normalized_path, DOCROOT);
    LOG_EXPR(&normalized_path);
    char  buf[PATH_MAX];
    char* normalized_cstr = str_cstr_buf(&normalized_path, buf);
    char* resolved_path_cstr = realpath(normalized_cstr, nullptr);
    if (!resolved_path_cstr) {
        goto ERR_404_NOT_FOUND;
    }
    String resolved_path = str_make_cstr(resolved_path_cstr);
    if (!str_hasPrefix(&resolved_path, RESOLVED_DOCROOT)) {
        goto ERR_403_FORBIDDEN;
    }

    //    StringView HttpResource = resolve_HttpResource(normalized_path);
    return res;
ERR_501_NOT_IMPLEMENTED:
    SET_ERROR(res, NOT_IMPLEMENTED);
    return res;

ERR_403_FORBIDDEN:
    SET_ERROR(res, FORBIDDEN);
    return res;
ERR_404_NOT_FOUND:
    SET_ERROR(res, NOT_FOUND);
    return res;
}

void* handle_client(void* arg) {
    // request-target = origin-form
    //
    Byte test_get_header_buf[] = "GET /?idkthisisaquery HTTP/1.1\r\n"
                                 "Host: example.com\r\n"
                                 "User-Agent: curl/8.6.0\r\n"
                                 "Accept: */*\r\n"
                                 "\r\n";

    //    Byte test_get_header_buf[] = "GET /abc%2C%2A%2B/def/idgaf?idkthisisaquery HTTP/1.1\r\n"
    //                                 "Host: example.com\r\n"
    //                                 "User-Agent: curl/8.6.0\r\n"
    //                                 "Accept: */*\r\n"
    //                                 "\r\n";

    int n_bytes_received = arrlen(test_get_header_buf);
    LOG_INFO("Recieved %zuB from client.", n_bytes_received);

    ByteStream   stream = bs_make(test_get_header_buf, n_bytes_received);
    HttpRequest  request = parse_HttpRequest(&stream);
    HttpResource resource = resolve_HttpRequest(request);
    // TODO: implement
    //    HttpResponse response = genereate_HttpResponse(resource);

    /*
     * INTENDED ORDER:
    HttpRequest  request  = parse_HttpRequest(&stream);
    HttpResource resource = resolveRequest(http_request);
    HttpResponse response = generateResponse(resource);
    Byte*        outgoing_data = encodeResponse(response);
        // HttpRequest ->  request_...()
        // HttpResource -> resource_...()
        // HttpResponse -> response_...()
    */

    return nullptr;
}

// sigint handler
void handle_SIGINT(int sig) {
    LOG_FATAL("<- handling SIGINT.\n");
    int opt = 1;
    setsockopt(config.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG_EXIT(EXIT_SUCCESS);
}

void init_config(int argc, char** argv) {
    int num_args = argc - 1;
    config.is_localhost = false;
    strcpy(config.hostname, "lmeldrum.dev");
    config.port = HTTP_TCP_PORT;
    config.sock_addr = (sockaddr_in){
        .sin_family = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port = htons(HTTP_TCP_PORT),
    };

    if (num_args == 1) {
        if (streq(argv[1], "-l") || streq(argv[1], "--local")) {
            config.is_localhost = true;
        } else if (streq(argv[1], "-h") || streq(argv[1], "-?") || streq(argv[1], "--help")) {
            LOG_INFO(STR_USAGE);
            LOG_EXIT(EXIT_SUCCESS);
        }
    } else if (num_args == 0) {
        config.is_localhost = false;
    } else {
        LOG_FATAL("Unknown args passed, usage: \n %s %s\n", argv[0], STR_USAGE);
    }

    // make alterations from default
    if (config.is_localhost) {
        LOG_INFO(BOLD "Localhost mode selected! localhost:random_port will be "
                      "used." FMT_CLEAR "\n\n");
        strcpy(config.hostname, "127.0.0.1");
        config.port = 49152;
        config.sock_addr.sin_port = htons(config.port);
    }
}

void handle_socket_bind_err(SOCK_STATUS bind_status) {
    int time_to_sleep = 25;
    LOG_ERROR("Port %hu failed to bind. \n", ntohs(config.sock_addr.sin_port));
    LOG_ERRNO();
    while (bind_status == SOCK_ERR) {
        LOG_INFO("Trying again in %ds... \n", time_to_sleep);
        sleep(time_to_sleep);
        bind_status =
            bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
        time_to_sleep *= 2;
    }
}

void init_socket(void) {
    config.server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (config.server_fd == SOCK_ERR) {
        LOG_FATAL("Unable to create socket.\n");
        LOG_EXIT(EXIT_FAILURE);
    }

    SOCK_STATUS bind_status =
        bind(config.server_fd, (sockaddr*)&config.sock_addr, sizeof(config.sock_addr));
    if (bind_status == SOCK_ERR)
        handle_socket_bind_err(bind_status);

    SOCK_STATUS listen_status = listen(config.server_fd, MAX_LISTENER_THREADS);
    if (listen_status == SOCK_ERR) {
        LOG_FATAL("Unable to start listening for connections.\n");
        LOG_EXIT(EXIT_FAILURE);
    }
}
