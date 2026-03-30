#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#include "http_consts.h"
#include "http_response.h"
#include "logger.h"
#include "myutils.h"
#include "str_extras.h"

void destroy_request(http_request req) { free(req.data); }
#define MAX_FILE_SZ (size_t)(1e9) // 1Gb ~(125MB)
#define MAX_RESPONSE_HEADER_SZ (size_t)1024

static char *parse_mime_type(char *filename);
static char *read_file(const char *filename, size_t file_size);
static http_response serialize_http_response(http_response_cfg c);

/* we use the windows style for completeness. */
#define NEWL "\r\n"
size_t syscall_file_size(const char *filename) {
    struct stat s;
    stat(filename, &s);
    return s.st_size;
}

/*
typedef struct http_request {
        char *data;
        size_t data_len;
        HTTP_METHOD method;
        HTTP_TARGET target;
        http_version version;
        HTTP_FIELD *fields;
        size_t nfields;
} http_request;
*/

#define MAX_VERSION_STRLEN 3
#define MAX_METHOD_STRLEN 32
#define MAX_TARGET_STRLEN 256

#define MAX_REQUEST_LINE_SZ                                                    \
    MAX_TARGET_STRLEN + MAX_VERSION_STRLEN + MAX_METHOD_STRLEN + 1

#define MAX_HEADER_FIELD_SZ 128
#define MAX_HEADER_VALUE_SZ 1024
// *INDENT-OFF*

static http_method parse_method(char *method_str);
static http_target parse_target(char *target_str);
static http_version parse_version(char *version_str);

http_request parse_http_request(char *data, size_t data_len) {
    http_request req = (http_request){.data = data, .data_len = data_len};

    char method_str[MAX_METHOD_STRLEN];
    char target_str[MAX_TARGET_STRLEN];
    char version_str[MAX_VERSION_STRLEN];

    int n =
        sscanf(data, "%s %s HTTP/%s" NEWL, method_str, target_str, version_str);
    if (n != 3) {
        LOG_FATAL("Unable to parse request.\n");
        goto PARSE_HTTP_RQ_ERROR;
    }

    req.method = parse_method(method_str);
    req.target = parse_target(target_str);
    req.version = parse_version(version_str);

    if (req.method == HTTP_METHOD_ERR) {
        LOG_FATAL("PARSING FAILURE: Method '%s' not recognised.\n", method_str);
        goto PARSE_HTTP_RQ_ERROR;
    }

    if (!req.target.filename || !req.target.mime_type) {
        LOG_FATAL("PARSING FAILURE: Filetype of '%s' not recognised.\n",
                  target_str);
        goto PARSE_HTTP_RQ_ERROR;
    }

    if (req.version != HTTP_VER_1_1) {
        LOG_FATAL("Version '%s' is not supported!\n", version_str);
        goto PARSE_HTTP_RQ_ERROR;
    }
    // for now we just ignore the header-fields because we dont need them
    return req;

PARSE_HTTP_RQ_ERROR:
    return (http_request){};
}

http_response build_response_to_GET(http_request request) {
    http_response_cfg GET_response_config;
    bool file_exists = access(request.target.filename, F_OK) == 0;
    size_t file_size = syscall_file_size(request.target.filename);
    char *file_contents = read_file(request.target.filename, file_size);

    if (!file_exists || !file_contents) {
        GET_response_config = HTTP_ERR_NOT_FOUND;
        return serialize_http_response(GET_response_config);
    }

    if (!request.target.mime_type) {
        GET_response_config = HTTP_ERR_BAD_REQUEST;
        return serialize_http_response(GET_response_config);
    }

    GET_response_config = (http_response_cfg){
        .version = REQUIRED_HTTP_VER_STR,
        .status = 200,
        .reason_phrase = "200: OK",
        .mime_type = request.target.mime_type,
        .msg_body = file_contents,
        .malloced_body = true,
        .body_is_txt = ((strstr(request.target.mime_type, "text/")) != NULL),
        .body_size = file_size,
    };
    return serialize_http_response(GET_response_config);
}

http_response build_http_response(http_request request) {
    LOG_INFO("building response for target --> '%s'\n",
             request.target.filename);
    switch (request.method) {
    case HTTP_GET:
        return build_response_to_GET(request);
        break;
    case HTTP_METHOD_ERR:
        break;
    default:
        break;
    }
    return (http_response){};
}

// *INDENT-OFF*
// caller responsible for freeing buffer; unless ret null
http_response serialize_http_response(http_response_cfg c) {
    const size_t MAX_RESPONSE_SZ = MAX_RESPONSE_HEADER_SZ + MAX_FILE_SZ;

    char *response_buf = calloc((MAX_RESPONSE_SZ + 1), sizeof(char));
    LOG_INFO("maxrespone=%zuB \n", MAX_RESPONSE_SZ);
    if (!response_buf) {
        LOG_FATAL("calloc failed.");
        return (http_response){};
    }

    snprintf(response_buf, MAX_RESPONSE_HEADER_SZ,
             "HTTP/%s %d %s" NEWL "Content-Type: %s" NEWL NEWL, c.version,
             c.status, c.reason_phrase, c.mime_type);

    if (!response_buf) {
        LOG_FATAL("Unable to write header contents to response buffer!\n");
        return (http_response){};
    }
    if (strnlen(response_buf, MAX_RESPONSE_HEADER_SZ) >=
        MAX_RESPONSE_HEADER_SZ) {
        LOG_FATAL(
            "Header contents are too long! May not have space for msg body.\n");
        return (http_response){};
    }

    size_t len = strlen(response_buf);
    if (c.body_is_txt) {
        strlcat(response_buf, c.msg_body, MAX_RESPONSE_SZ + 1);
    } else {
        memcpy((response_buf + len), c.msg_body, c.body_size);
    }

    if (c.malloced_body)
        free(c.msg_body);
    return (http_response){
        .data = response_buf,
        .len = c.body_size + len,
    };
}

#define logretnull(fmt, ...)                                                   \
    do {                                                                       \
        LOG_FATAL(fmt, ##__VA_ARGS__) return NULL;                             \
    } while (0)

char *read_file(const char *filename, size_t file_size) {
    FILE *file_ptr = fopen(filename, "rb");

    if (!file_ptr) {
        LOG_FATAL("Unable to open file '%s'.\n", filename);
        return NULL;
    }

    if (file_size == 0) {
        LOG_FATAL("syscall_file_size(%s) ret=0.\n", filename);
        fclose(file_ptr);
        return NULL;
    }

    if (file_size > MAX_FILE_SZ) {
        LOG_FATAL("Requested file '%s' is too large! (%zu>%zu)\n", filename,
                  file_size, MAX_FILE_SZ);
        fclose(file_ptr);
        return NULL;
    }
    char *file_contents = malloc(sizeof(char) * file_size);
    if (!file_contents) {
        LOG_FATAL("Unable to alloc buffer for file '%s'.\n", filename);
        fclose(file_ptr);
        return NULL;
    }

    int n_read = fread(file_contents, file_size, 1, file_ptr);
    if (n_read != 1) {
        LOG_FATAL("Unable to read file contents for file '%s'.\n", filename);
        free(file_contents);
        fclose(file_ptr);
        return NULL;
    }
    fclose(file_ptr);
    return file_contents;
}

char *parse_file_extension(char *filename) {
    char *first_dot = strchr(filename, '.');
    if (strchr(first_dot + 1, '.')) {
        return NULL;
    } else {
        return first_dot;
    }
}

char *parse_mime_type(char *filename) {
    char *ext = parse_file_extension(filename);
    if (!ext) {
        LOG_FATAL("GET Request for file (%s) has multiple '.' chars,"
                  "unable to determine MIME type!\n",
                  filename);
        return NULL;
    }
    if (*ext == '\0' || *(ext + 1) == '\0') {
        LOG_FATAL("GET request for file (%s) has no file extension,"
                  "unable to determine MIME type!\n",
                  filename);
        return NULL;
    }
    // extend as necessary
    if (streq(ext, ".bin"))
        return "application/octet-stream";
    if (streq(ext, ".css"))
        return "application/octet-stream";
    if (streq(ext, ".aac"))
        return "audio/aac";
    if (streq(ext, ".abw"))
        return "application/x-abiword";
    if (streq(ext, ".apng"))
        return "image/apng";
    if (streq(ext, ".arc"))
        return "application/x-freearc";
    if (streq(ext, ".avif"))
        return "image/avif";
    if (streq(ext, ".avi"))
        return "video/x-msvideo";
    if (streq(ext, ".azw"))
        return "application/vnd.amazon.ebook";
    if (streq(ext, ".bin"))
        return "application/octet-stream";
    if (streq(ext, ".bmp"))
        return "image/bmp";
    if (streq(ext, ".bz"))
        return "application/x-bzip";
    if (streq(ext, ".bz2"))
        return "application/x-bzip2";
    if (streq(ext, ".cda"))
        return "application/x-cdf";
    if (streq(ext, ".csh"))
        return "application/x-csh";
    if (streq(ext, ".css"))
        return "text/css";
    if (streq(ext, ".csv"))
        return "text/csv";
    if (streq(ext, ".doc"))
        return "application/msword";
    if (streq(ext, ".docx"))
        return "application/"
               "vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (streq(ext, ".eot"))
        return "application/vnd.ms-fontobject";
    if (streq(ext, ".epub"))
        return "application/epub+zip";
    if (streq(ext, ".gz"))
        return "application/gzip";
    if (streq(ext, ".gif"))
        return "image/gif";
    if (streq(ext, ".ico"))
        return "image/vnd.microsoft.icon";
    if (streq(ext, ".ics"))
        return "text/calendar";
    if (streq(ext, ".jar"))
        return "application/java-archive";
    if (streq(ext, ".js"))
        return "text/javascript";
    if (streq(ext, ".json"))
        return "application/json";
    if (streq(ext, ".jsonld"))
        return "application/ld+json";
    if (streq(ext, ".md"))
        return "text/markdown";
    if (streq(ext, ".mjs"))
        return "text/javascript";
    if (streq(ext, ".mp3"))
        return "audio/mpeg";
    if (streq(ext, ".mp4"))
        return "video/mp4";
    if (streq(ext, ".mpeg"))
        return "video/mpeg";
    if (streq(ext, ".mpkg"))
        return "application/vnd.apple.installer+xml";
    if (streq(ext, ".odp"))
        return "application/vnd.oasis.opendocument.presentation";
    if (streq(ext, ".ods"))
        return "application/vnd.oasis.opendocument.spreadsheet";
    if (streq(ext, ".odt"))
        return "application/vnd.oasis.opendocument.text";
    if (streq(ext, ".oga"))
        return "audio/ogg";
    if (streq(ext, ".ogv"))
        return "video/ogg";
    if (streq(ext, ".ogx"))
        return "application/ogg";
    if (streq(ext, ".opus"))
        return "audio/ogg";
    if (streq(ext, ".otf"))
        return "font/otf";
    if (streq(ext, ".png"))
        return "image/png";
    if (streq(ext, ".pdf"))
        return "application/pdf";
    if (streq(ext, ".php"))
        return "application/x-httpd-php";
    if (streq(ext, ".ppt"))
        return "application/vnd.ms-powerpoint";
    if (streq(ext, ".pptx"))
        return "application/"
               "vnd.openxmlformats-officedocument.presentationml.presentation";
    if (streq(ext, ".rar"))
        return "application/vnd.rar";
    if (streq(ext, ".rtf"))
        return "application/rtf";
    if (streq(ext, ".sh"))
        return "application/x-sh";
    if (streq(ext, ".svg"))
        return "image/svg+xml";
    if (streq(ext, ".tar"))
        return "application/x-tar";
    if (streq(ext, ".ts"))
        return "video/mp2t";
    if (streq(ext, ".ttf"))
        return "font/ttf";
    if (streq(ext, ".txt"))
        return "text/plain";
    if (streq(ext, ".vsd"))
        return "application/vnd.visio";
    if (streq(ext, ".wav"))
        return "audio/wav";
    if (streq(ext, ".weba"))
        return "audio/webm";
    if (streq(ext, ".webm"))
        return "video/webm";
    if (streq(ext, ".webmani"))
        return "festapplication/manifest+json";
    if (streq(ext, ".webp"))
        return "image/webp";
    if (streq(ext, ".woff"))
        return "font/woff";
    if (streq(ext, ".woff2"))
        return "font/woff2";
    if (streq(ext, ".xhtml"))
        return "application/xhtml+xml";
    if (streq(ext, ".xls"))
        return "application/vnd.ms-excel";
    if (streq(ext, ".xlsx"))
        return "application/"
               "vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (streq(ext, ".xml"))
        return "application/xml";
    if (streq(ext, ".xul"))
        return "application/vnd.mozilla.xul+xml";
    if (streq(ext, ".zip"))
        return "application/zip";
    if (streq(ext, ".3gp"))
        return "video/3gpp";
    if (streq(ext, ".3g2"))
        return "video/3gpp2";
    if (streq(ext, ".7z"))
        return "application/x-7z-compressed";
    if (streq(ext, ".tif"))
        return "image/tiff";
    if (streq(ext, ".tiff"))
        return "image/tiff";
    if (streq(ext, ".htm"))
        return "text/html";
    if (streq(ext, ".html"))
        return "text/html";
    if (streq(ext, ".jpeg"))
        return "image/jpeg";
    if (streq(ext, ".jpg"))
        return "image/jpeg";
    if (streq(ext, ".mid"))
        return "audio/midi";
    if (streq(ext, ".midi"))
        return "audio/midi";
    return NULL;
}

http_method parse_method(char *method_str) {
    if (!method_str)
        return HTTP_METHOD_ERR;

    if (streq(method_str, "GET"))
        return HTTP_GET;
    if (streq(method_str, "HEAD"))
        return HTTP_HEAD;
    else
        return HTTP_METHOD_ERR;
}

http_target parse_target(char *req_path_raw) {
    LOG_DEBUG("Request made for raw path: '%s'", req_path_raw);
    // browser has made a request for a path.
    if (!req_path_raw || req_path_raw[0] != '/') {
        return (http_target){};
    }
    // 1. Append the true document root to the beginning of the req
    const char *req_path_no_lead_slash = req_path_raw + 1;
    size_t len = strnlen(req_path_raw, PATH_MAX);
    snprintf(req_path_raw, len, "%s/%s", document_root, req_path_no_lead_slash);

    // 2. resolve any symlinks, ../ or ./ in the path.
    char req_path_resolved[PATH_MAX];
    if (!realpath(req_path_raw, req_path_resolved)) {
        LOG_INFO("%s could not be req_path_resolved to a target.\n",
                 req_path_raw);
        return (http_target){};
    }
    // 3. ensure that after resolution, the request path still begins with the
    // document root (avoids the case of ../ escaping root dir)
    if (!cstr_startsWith(req_path_resolved, document_root)) {
        LOG_INFO("%s does not begin with %s!\n", req_path_resolved,
                 document_root);
        return (http_target){};
    }
    if (streq(req_path_resolved, document_root)) {
        strcpy(req_path_resolved, "/index.html");
    }
    LOG_DEBUG("Request resolved to: '%s'", req_path_resolved);

    return (http_target){
        .filename = req_path_resolved,
        .mime_type = parse_mime_type(req_path_resolved),
    };
}

http_version parse_version(char *version_str) {
    if (!version_str)
        return HTTP_VER_ERR;

    if (streq(version_str, "1.1"))
        return HTTP_VER_1_1;
    return HTTP_VER_ERR;
}
