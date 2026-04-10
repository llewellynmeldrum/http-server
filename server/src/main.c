#include <arpa/inet.h>
#include <assert.h>
#include <limits.h>
#include <netinet/in.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <unistd.h>

#include "ByteStream.h"
#include "Globals.h"
#include "HashMap.h"
#include "HttpError.h"
#include "HttpMetadata.h"
#include "HttpParsing.h"
#include "HttpTarget.h"
#include "LString.h"
#include "ServerTypes.h"
#include "SocketWrapper.h"
#include "StringView.h"

#include "HttpRequest.h"
#include "macromagic.h"

pthread_t client_handler_thread;

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

#define static_strlen(cstr) (sizeof(cstr) - 1)

#define DECL_STRINGVIEW(cstr)                                                                      \
    (StringView) {                                                                                 \
        .ptr = cstr, .len = static_strlen(cstr)                                                    \
    }

#define DECL_LONG_STRING(cstr)                                                                     \
    (String) {                                                                                     \
        .ptr = cstr, .len = (static_strlen(cstr)), .cap = static_strlen(cstr)                      \
    }
#define DECL_STRING(cstr) DECL_LONG_STRING(cstr)
// clang-format off
String mimetype_vals[] = {
    DECL_STRING("application/octet-stream"),
    DECL_STRING("application/octet-stream"),
    DECL_STRING("audio/aac"),
    DECL_STRING("application/x-abiword"),
    DECL_STRING("image/apng"),
    DECL_STRING("application/x-freearc"),
    DECL_STRING("image/avif"),
    DECL_STRING("video/x-msvideo"),
    DECL_STRING("application/vnd.amazon.ebook"),
    DECL_STRING("application/octet-stream"),
    DECL_STRING("image/bmp"),
    DECL_STRING("application/x-bzip"),
    DECL_STRING("application/x-bzip2"),
    DECL_STRING("application/x-cdf"),
    DECL_STRING("application/x-csh"),
    DECL_STRING("text/css"),
    DECL_STRING("text/csv"),
    DECL_STRING("application/msword"),
    DECL_STRING("application/vnd.openxmlformats-officedocument.wordprocessingml.document"),
    DECL_STRING("application/vnd.ms-fontobject"),
    DECL_STRING("application/epub+zip"),
    DECL_STRING("application/gzip"),
    DECL_STRING("image/gif"),
    DECL_STRING("image/vnd.microsoft.icon"),
    DECL_STRING("text/calendar"),
    DECL_STRING("application/java-archive"),
    DECL_STRING("text/javascript"),
    DECL_STRING("application/json"),
    DECL_STRING("application/ld+json"),
    DECL_STRING("text/markdown"),
    DECL_STRING("text/javascript"),
    DECL_STRING("audio/mpeg"),
    DECL_STRING("video/mp4"),
    DECL_STRING("video/mpeg"),
    DECL_STRING("application/vnd.apple.installer+xml"),
    DECL_STRING("application/vnd.oasis.opendocument.presentation"),
    DECL_STRING("application/vnd.oasis.opendocument.spreadsheet"),
    DECL_STRING("application/vnd.oasis.opendocument.text"),
    DECL_STRING("audio/ogg"),
    DECL_STRING("video/ogg"),
    DECL_STRING("application/ogg"),
    DECL_STRING("audio/ogg"),
    DECL_STRING("font/otf"),
    DECL_STRING("image/png"),
    DECL_STRING("application/pdf"),
    DECL_STRING("application/x-httpd-php"),
    DECL_STRING("application/vnd.ms-powerpoint"),
    DECL_STRING("application/vnd.openxmlformats-officedocument.presentationml.presentation"),
    DECL_STRING("application/vnd.rar"),
    DECL_STRING("application/rtf"),
    DECL_STRING("application/x-sh"),
    DECL_STRING("image/svg+xml"),
    DECL_STRING("application/x-tar"),
    DECL_STRING("video/mp2t"),
    DECL_STRING("font/ttf"),
    DECL_STRING("text/plain"),
    DECL_STRING("application/vnd.visio"),
    DECL_STRING("audio/wav"),
    DECL_STRING("audio/webm"),
    DECL_STRING("video/webm"),
    DECL_STRING("festapplication/manifest+json"),
    DECL_STRING("image/webp"),
    DECL_STRING("font/woff"),
    DECL_STRING("font/woff2"),
    DECL_STRING("application/xhtml+xml"),
    DECL_STRING("application/vnd.ms-excel"),
    DECL_STRING("application/vnd.openxmlformats-officedocument.spreadsheetml.sheet"),
    DECL_STRING("application/xml"),
    DECL_STRING("application/vnd.mozilla.xul+xml"),
    DECL_STRING("application/zip"),
    DECL_STRING("video/3gpp"),
    DECL_STRING("video/3gpp2"),
    DECL_STRING("application/x-7z-compressed"),
    DECL_STRING("image/tiff"),
    DECL_STRING("image/tiff"),
    DECL_STRING("text/html"),
    DECL_STRING("text/html"),
    DECL_STRING("image/jpeg"),
    DECL_STRING("image/jpeg"),
    DECL_STRING("audio/midi"),
    DECL_STRING("audio/midi"),
};

StringView mimetype_keys[] = {
	DECL_STRINGVIEW(".bin"),
	DECL_STRINGVIEW(".css"),
	DECL_STRINGVIEW(".aac"),
	DECL_STRINGVIEW(".abw"),
	DECL_STRINGVIEW(".apng"),
	DECL_STRINGVIEW(".arc"),
	DECL_STRINGVIEW(".avif"),
	DECL_STRINGVIEW(".avi"),
	DECL_STRINGVIEW(".azw"),
	DECL_STRINGVIEW(".bin"),
	DECL_STRINGVIEW(".bmp"),
	DECL_STRINGVIEW(".bz"),
	DECL_STRINGVIEW(".bz2"),
	DECL_STRINGVIEW(".cda"),
	DECL_STRINGVIEW(".csh"),
	DECL_STRINGVIEW(".css"),
	DECL_STRINGVIEW(".csv"),
	DECL_STRINGVIEW(".doc"),
	DECL_STRINGVIEW(".docx"),
	DECL_STRINGVIEW(".eot"),
	DECL_STRINGVIEW(".epub"),
	DECL_STRINGVIEW(".gz"),
	DECL_STRINGVIEW(".gif"),
	DECL_STRINGVIEW(".ico"),
	DECL_STRINGVIEW(".ics"),
	DECL_STRINGVIEW(".jar"),
	DECL_STRINGVIEW(".js"),
	DECL_STRINGVIEW(".json"),
	DECL_STRINGVIEW(".jsonld"),
	DECL_STRINGVIEW(".md"),
	DECL_STRINGVIEW(".mjs"),
	DECL_STRINGVIEW(".mp3"),
	DECL_STRINGVIEW(".mp4"),
	DECL_STRINGVIEW(".mpeg"),
	DECL_STRINGVIEW(".mpkg"),
	DECL_STRINGVIEW(".odp"),
	DECL_STRINGVIEW(".ods"),
	DECL_STRINGVIEW(".odt"),
	DECL_STRINGVIEW(".oga"),
	DECL_STRINGVIEW(".ogv"),
	DECL_STRINGVIEW(".ogx"),
	DECL_STRINGVIEW(".opus"),
	DECL_STRINGVIEW(".otf"),
	DECL_STRINGVIEW(".png"),
	DECL_STRINGVIEW(".pdf"),
	DECL_STRINGVIEW(".php"),
	DECL_STRINGVIEW(".ppt"),
	DECL_STRINGVIEW(".pptx"),
	DECL_STRINGVIEW(".rar"),
	DECL_STRINGVIEW(".rtf"),
	DECL_STRINGVIEW(".sh"),
	DECL_STRINGVIEW(".svg"),
	DECL_STRINGVIEW(".tar"),
	DECL_STRINGVIEW(".ts"),
	DECL_STRINGVIEW(".ttf"),
	DECL_STRINGVIEW(".txt"),
	DECL_STRINGVIEW(".vsd"),
	DECL_STRINGVIEW(".wav"),
	DECL_STRINGVIEW(".weba"),
	DECL_STRINGVIEW(".webm"),
	DECL_STRINGVIEW(".webmani"),
	DECL_STRINGVIEW(".webp"),
	DECL_STRINGVIEW(".woff"),
	DECL_STRINGVIEW(".woff2"),
	DECL_STRINGVIEW(".xhtml"),
	DECL_STRINGVIEW(".xls"),
	DECL_STRINGVIEW(".xlsx"),
	DECL_STRINGVIEW(".xml"),
	DECL_STRINGVIEW(".xul"),
	DECL_STRINGVIEW(".zip"),
	DECL_STRINGVIEW(".3gp"),
	DECL_STRINGVIEW(".3g2"),
	DECL_STRINGVIEW(".7z"),
	DECL_STRINGVIEW(".tif"),
	DECL_STRINGVIEW(".tiff"),
	DECL_STRINGVIEW(".htm"),
	DECL_STRINGVIEW(".html"),
	DECL_STRINGVIEW(".jpeg"),
	DECL_STRINGVIEW(".jpg"),
	DECL_STRINGVIEW(".mid"),
	DECL_STRINGVIEW(".midi"),
};
// clang-format on

static HashMap mimetype_map;  // <StringView, StringView>
void           init_mimetypeMap(void) {
    mimetype_map = hm_make(sv_hash, sv_hash_equal);

    static_assert(arrlen(mimetype_keys) == arrlen(mimetype_vals),
                  "Arrays must be of the same length");

    for (size_t i = 0; i < arrlen(mimetype_keys); i++) {
        hm_insert(&mimetype_map, &mimetype_keys[i], &mimetype_vals[i]);
    }
}
static void init_static_data(void) {
    init_percentEncodingMap();
    init_mimetypeMap();
}
u64  program_epoch_ns;
char document_root[PATH_MAX] = {};

int main(int argc, char** argv) {
    signal(SIGINT, handle_SIGINT);
    program_epoch_ns = get_current_ns();
    init_static_data();

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

struct HttpResponse {
    const HttpVersion version;

    String      resource_path;
    HttpStatus  status;
    HttpHeader* headers;
    size_t      header_count;

    size_t resource_size;
    FILE*  resource_fptr;
};
typedef struct HttpResponse HttpResponse;

static void get_current_gmtime_str(char* buf, size_t len) {
    time_t     raw_time;
    struct tm* time_info;
    time(&raw_time);
    time_info = gmtime(&raw_time);
    strftime(buf, len, "%a, %d %b %Y %H:%M:%S GMT", time_info);
}

static String HEADER_NAME_Date = DECL_STRING("Date");
static String HEADER_NAME_Content_Type = DECL_STRING("Content-Type");
static String HEADER_NAME_Content_Length = DECL_STRING("Content-Length");
HttpResponse  generate_HttpResponse(HttpTarget target) {
    // given some resource (which is essentially a resolved file path on the servers machine),
    HttpResponse res = {
        .version = HttpVersion_1_1,
        .resource_path = target.resolved_path,
    };
    if (target.hasError) {
        res.status = target.err.code;
        res.headers = nullptr;
        res.header_count = 0;
        res.resource_size = 0;
        res.resource_fptr = nullptr;
        return res;
    }
    char path_cstr[PATH_MAX] = {};
    str_cstr_buf(&res.resource_path, path_cstr);

    struct stat st;
    if (stat(path_cstr, &st) != 0) {
        goto ERR_404_NOT_FOUND;
    }
    res.resource_size = st.st_size;
    if (res.resource_size >= MAX_FILE_SZ) {
        goto ERR_413_CONTENT_TOO_LARGE;
    }

    constexpr size_t MAX_RESPONSE_HEADERS = 32;
    HttpHeader       header_buf[MAX_RESPONSE_HEADERS] = {};
    size_t           header_count = 0;

    // NOTE: HEADER 1
    // date: <gmt formatted date>
    char buffer[80];
    get_current_gmtime_str(buffer, arrlen(buffer));
    printf("time: '%s'\n", buffer);
    String date_value = str_make_cstr(buffer);
    header_buf[header_count++] = responseHeader_make(HEADER_NAME_Date, date_value);

    // NOTE: HEADER 2
    // content-type: <mime-type>
    size_t     last_dot = str_rfind(&res.resource_path, '.');
    StringView file_ext = str_slice(&res.resource_path, last_dot, STR_END);
    String     mimetype = *(String*)hm_find(&mimetype_map, &file_ext);
    //    LOG_EXPR(file_ext);
    //    LOG_EXPR(&mimetype);
    header_buf[header_count++] = responseHeader_make(HEADER_NAME_Content_Type, mimetype);

    // NOTE: HEADER 3
    // content-length: <body-length>
    String body_size_str = str_itos(res.resource_size);
    //    LOG_EXPR(st.st_size);

    //   LOG_EXPR(&body_size_str);
    header_buf[header_count++] = responseHeader_make(HEADER_NAME_Content_Length, body_size_str);

    // NOTE: Allocate proper header buffer
    res.headers = calloc(sizeof(HttpHeader), header_count);
    for (size_t i = 0; i < header_count; i++) {
        memcpy(&res.headers[i], &header_buf[i], sizeof(HttpHeader));
        char* str = header_toStr(res.headers[i]);
        LOG_DEBUG("[%zu]='%s'", i, str);
        free(str);
    }
    res.header_count = header_count;
    res.resource_fptr = fopen(path_cstr, "rb");
    res.status = HttpStatus_OK;
    return res;

ERR_404_NOT_FOUND:
    SET_RESP_ERROR(res, NOT_FOUND);
    return res;
ERR_413_CONTENT_TOO_LARGE:
    SET_RESP_ERROR(res, CONTENT_TOO_LARGE);
    return res;
}

// fat pointer style array
struct ByteArray {
    Byte*  ptr;
    size_t len;
};
typedef struct ByteArray ByteArray;
ByteArray                serialize_HttpResponse(HttpResponse resp) {
    // status-line = HTTP-version SP status-code SP [ reason-phrase ] CRLF
    String res = str_make_empty();
    str_append_cstr(&res, HttpVersion_toStr[resp.version]);
    str_append_cstr(&res, " ");
    str_append_cstr(&res, HttpStatus_toStr[resp.status]);
    str_append_cstr(&res, " ");
    str_append_sv(&res, CRLF);

    for (size_t i = 0; i < resp.header_count; i++) {
        char* str = header_toStr(resp.headers[i]);
        LOG_DEBUG("[%zu]='%s'", i, str);
        free(str);
    }
    // *( field-line CRLF )
    //                CRLF
    for (size_t i = 0; i < resp.header_count; i++) {
        LOG_EXPR(&resp.headers[i].response_name);
        str_append_str(&res, &resp.headers[i].response_name);
        str_append_sv(&res, COLON);
        LOG_EXPR(&resp.headers[i].response_val);
        str_append_str(&res, &resp.headers[i].response_val);
    }
    LOG_EXPR(&res);
    return (ByteArray){};
}
void       init_percentEncodingMap(void);
HttpTarget resolve_HttpRequest(HttpRequest request);

void HttpRequest_toStr(HttpRequest req, char* buf, size_t buflen) {
    snprintf(buf,
             BUF_SZ,
             "HttpRequest {\n"
             "    .method     = '%s'\n"
             "    .target_sv  = '%s'\n"
             "    .version    = '%s'\n"
             "    .headers    = %s\n"
             "} HttpRequest;",
             HttpRequestMethod_toStr[req.method],
             sv_cstr(req.target_sv),
             HttpVersion_toStr[req.version],
             header_array_ToStr(req.headers, req.num_headers));
}
#define TEST_HTTP_REQUEST(data) _TEST_HTTP_REQUEST(#data, data, arrlen(data))
ByteArray _TEST_HTTP_REQUEST(const char* name, Byte* buf, int buf_len) {
    LOG_INFO("TEST(%s) -> %zuB:", name, buf_len);
    ByteStream  stream = bs_make(buf, buf_len);
    HttpRequest request = parse_HttpRequest(&stream);
    char        req_cstr[BUF_SZ] = {};
    HttpRequest_toStr(request, req_cstr, BUF_SZ);
    LOG_DEBUG("%s", req_cstr);
    HttpTarget   target = resolve_HttpRequest(request);
    HttpResponse response = generate_HttpResponse(target);
    ByteArray    outgoing_data = serialize_HttpResponse(response);
    /* INTENDED ORDER:
    HttpRequest  request  = parse_HttpRequest(&stream);
    HttpTarget resource = resolveRequest(http_request);
    HttpResponse response = generateResponse(resource);
    Byte*        outgoing_data = encodeResponse(response);
    */
    printf("\n");
    return outgoing_data;
}
void* handle_client(void* arg) {
    // TEST(percent_escapes)

    // TODO: I have ported the percent encoding map over to the new map,
    // now i need to construct the other hashmap for mime types.

    //"%20%21%23%24%25%26%27%28%29%2A%2B%2C%2F%3A%3B%3D%3F%5B%5D%40";
    Byte origin_GET[] = "GET /?idkthisisaquery "
                        "HTTP/1.1\r\n"
                        "Host: example.com\r\n"
                        "User-Agent: curl/8.6.0\r\n"
                        "Accept: */*\r\n"
                        "\r\n";
    Byte absolute_GET[] = "GET http://lmeldrum.dev/pathway/remove_me/../ HTTP/1.1\r\n"
                          "Host: example.com\r\n"
                          "User-Agent: curl/8.6.0\r\n"
                          "Accept: */*\r\n"
                          "\r\n";
    Byte absolute_GET_BAD_HOST[] = "GET http://google.com/pathway/remove_me/../ HTTP/1.1\r\n"
                                   "Host: google.com\r\n"
                                   "User-Agent: curl/8.6.0\r\n"
                                   "Accept: */*\r\n"
                                   "\r\n";

    Byte origin_GET_ESCAPE_DOCROOT[] = "GET /test/test/test/../../../../ HTTP/1.1\r\n"
                                       "Host: example.com\r\n"
                                       "User-Agent: curl/8.6.0\r\n"
                                       "Accept: */*\r\n"
                                       "\r\n";
    Byte absolute_GET_ESCAPE_DOCROOT[] =
        "GET http://lmeldrum.dev/dir1/dir2/../dir2/../../../ HTTP/1.1\r\n"
        "Host: example.com\r\n"
        "User-Agent: curl/8.6.0\r\n"
        "Accept: */*\r\n"
        "\r\n";
    TEST_HTTP_REQUEST(origin_GET);
    // TEST_HTTP_REQUEST(absolute_GET);
    // TEST_HTTP_REQUEST(absolute_GET_BAD_HOST);
    // TEST_HTTP_REQUEST(origin_GET_ESCAPE_DOCROOT);
    // TEST_HTTP_REQUEST(absolute_GET_ESCAPE_DOCROOT);

    // TODO: Refactor some of the header only mess ive got going on.
    //       Move each stage to a separate module (.c/.h pair)
    return nullptr;
}

// sigint handler
void handle_SIGINT(int sig) {
    LOG_FATAL("<- handling SIGINT.\n");
    int opt = 1;
    setsockopt(config.server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
    LOG_EXIT(EXIT_SUCCESS);
    exit(EXIT_SUCCESS);
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
