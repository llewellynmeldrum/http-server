#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "file_handling.h"
#include "http_consts.h"
#include "http_response.h"
#include "logger.h"
#include "myutils.h"
#include "str_extras.h"

#include "http_response_internal.h"

HTTP_Request parse_http_request(char* data, size_t data_len) {
    HTTP_Request req = (HTTP_Request){ .data = data, .data_len = data_len };

    char method_str[MAX_METHOD_STRLEN];
    char target_str[MAX_TARGET_STRLEN];
    char version_str[MAX_VERSION_STRLEN];

    int n = sscanf(data, "%s %s HTTP/%s" NEWL, method_str, target_str, version_str);
    if (n != 3) {
        LOG_FATAL("Unable to parse request.\n");
        goto parse_http_request_CLEANUP;
    }
    /* else {

         LOG_DEBUG("method='%s' target='%s', version='%s'" NEWL, method_str, target_str,
                   version_str);
     }
    */

    req.method = parse_method(method_str);
    req.target = parse_target(target_str);
    req.version = parse_version(version_str);

    if (req.method == HTTP_METHOD_ERR) {
        LOG_FATAL("PARSING FAILURE: Method '%s' not recognised.\n", method_str);
        goto parse_http_request_CLEANUP;
    }

    if (!req.target.filename || !req.target.mime_type) {
        LOG_FATAL("File '%s' may not have been found Returning 404.\n", target_str);
        goto parse_http_request_CLEANUP;
    }

    if (req.version != HTTP_VER_1_1) {
        LOG_FATAL("Version '%s' is not supported!\n", version_str);
        goto parse_http_request_CLEANUP;
    }
    // for now we just ignore the header-fields because we dont need them
    return req;

parse_http_request_CLEANUP:
    return (HTTP_Request){
        .method = HTTP_METHOD_ERR,
    };
}

HTTP_Response build_response_to_GET(HTTP_Request request) {
    HTTP_ResponseConfig GET_response_config = {};

    bool file_exists = access(request.target.filename, F_OK) == 0;
    if (!file_exists) {
        GET_response_config = HTTP_ResponseConfig_ERR_NOTFOUND;
        return serialize_http_response(GET_response_config);
    }
    off_t file_size = syscall_file_size(request.target.filename);
    if (file_size == 0) {
        LOG_DEBUG("Requested file '%s' exists but has invalid file size (%ld)",
                  request.target.filename, file_size);
    }
    char* file_contents = read_file(request.target.filename, file_size);

    if (!file_exists || !file_contents) {
        GET_response_config = HTTP_ResponseConfig_ERR_NOTFOUND;
        return serialize_http_response(GET_response_config);
    }

    if (!request.target.mime_type) {
        GET_response_config = HTTP_ResponseConfig_ERR_BADREQUEST;
        return serialize_http_response(GET_response_config);
    }

    GET_response_config = (HTTP_ResponseConfig){
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

HTTP_Response build_http_response(HTTP_Request request) {
    LOG_INFO("building response for target --> '%s'\n", request.target.filename);
    if (request.method == HTTP_GET) {
        return build_response_to_GET(request);
    } else {
        return serialize_http_response(HTTP_ResponseConfig_ERR_NOTFOUND);
    }
}

// *INDENT-OFF*
// caller responsible for freeing buffer; unless ret null
HTTP_Response serialize_http_response(HTTP_ResponseConfig c) {
    const size_t MAX_RESPONSE_SZ = MAX_RESPONSE_HEADER_SZ + MAX_FILE_SZ;

    char* response_buf = calloc((MAX_RESPONSE_SZ + 1), sizeof(char));
    if (!response_buf) {
        LOG_FATAL("calloc failed.");
        return (HTTP_Response){};
    }

    snprintf(response_buf, MAX_RESPONSE_HEADER_SZ,
             "HTTP/%s %d %s" NEWL "Content-Type: %s" NEWL NEWL, c.version, c.status,
             c.reason_phrase, c.mime_type);

    LOG_DEBUG("serializing response: \n%s", response_buf);
    if (!response_buf) {
        LOG_FATAL("Unable to write header contents to response buffer!\n");
        return (HTTP_Response){};
    }
    if (strnlen(response_buf, MAX_RESPONSE_HEADER_SZ) >= MAX_RESPONSE_HEADER_SZ) {
        LOG_FATAL("Header contents are too long! May not have space for msg body.\n");
        return (HTTP_Response){};
    }

    size_t len = strlen(response_buf);
    if (c.body_is_txt) {
        strlcat(response_buf, c.msg_body, MAX_RESPONSE_SZ + 1);
    } else {
        memcpy((response_buf + len), c.msg_body, c.body_size);
    }

    if (c.malloced_body)
        free(c.msg_body);
    return (HTTP_Response){
        .data = response_buf,
        .len = c.body_size + len,
    };
}

char* parse_mime_type(char* filename) {
    char* file_ext = parse_file_extension(filename);
    if (!file_ext) {
        // file extension is nullptr.
        // Check if last char is /
        // Hello0
        // 012345
        const char last_char = filename[strlen(filename) - 1];
        if (last_char == '/') {
            // They are asking for a directory, meaning they probably want DIR/index.html
            char temp[PATH_MAX] = {};
            // copy filename into temp buffer
            strncpy(temp, filename, PATH_MAX);
            // temp += index.html suffix
            strcat(temp, "index.html");
            // ensure its null terminated
            temp[PATH_MAX - 1] = '\0';
            // copy buffer back into filename
            strncpy(filename, temp, PATH_MAX);
        } else {
            LOG_FATAL("GET Request for file (%s) failed,unable to parse file extension. !",
                      filename);
            return nullptr;
        }
    }
    LOG_DEBUG("Extension for '%s'='%s'", filename, file_ext);
    if (*file_ext == '\0' || *(file_ext + 1) == '\0') {
        LOG_FATAL("GET request for file (%s) has no file extension,"
                  "unable to determine MIME type!\n",
                  filename);
        return nullptr;
    }
    // extend as necessary
    if (streq(file_ext, ".bin"))
        return "application/octet-stream";
    if (streq(file_ext, ".aac"))
        return "audio/aac";
    if (streq(file_ext, ".abw"))
        return "application/x-abiword";
    if (streq(file_ext, ".apng"))
        return "image/apng";
    if (streq(file_ext, ".arc"))
        return "application/x-freearc";
    if (streq(file_ext, ".avif"))
        return "image/avif";
    if (streq(file_ext, ".avi"))
        return "video/x-msvideo";
    if (streq(file_ext, ".azw"))
        return "application/vnd.amazon.ebook";
    if (streq(file_ext, ".bin"))
        return "application/octet-stream";
    if (streq(file_ext, ".bmp"))
        return "image/bmp";
    if (streq(file_ext, ".bz"))
        return "application/x-bzip";
    if (streq(file_ext, ".bz2"))
        return "application/x-bzip2";
    if (streq(file_ext, ".cda"))
        return "application/x-cdf";
    if (streq(file_ext, ".csh"))
        return "application/x-csh";
    if (streq(file_ext, ".css"))
        return "text/css";
    if (streq(file_ext, ".csv"))
        return "text/csv";
    if (streq(file_ext, ".doc"))
        return "application/msword";
    if (streq(file_ext, ".docx"))
        return "application/"
               "vnd.openxmlformats-officedocument.wordprocessingml.document";
    if (streq(file_ext, ".eot"))
        return "application/vnd.ms-fontobject";
    if (streq(file_ext, ".epub"))
        return "application/epub+zip";
    if (streq(file_ext, ".gz"))
        return "application/gzip";
    if (streq(file_ext, ".gif"))
        return "image/gif";
    if (streq(file_ext, ".ico"))
        return "image/vnd.microsoft.icon";
    if (streq(file_ext, ".ics"))
        return "text/calendar";
    if (streq(file_ext, ".jar"))
        return "application/java-archive";
    if (streq(file_ext, ".js"))
        return "text/javascript";
    if (streq(file_ext, ".json"))
        return "application/json";
    if (streq(file_ext, ".jsonld"))
        return "application/ld+json";
    if (streq(file_ext, ".md"))
        return "text/markdown";
    if (streq(file_ext, ".mjs"))
        return "text/javascript";
    if (streq(file_ext, ".mp3"))
        return "audio/mpeg";
    if (streq(file_ext, ".mp4"))
        return "video/mp4";
    if (streq(file_ext, ".mpeg"))
        return "video/mpeg";
    if (streq(file_ext, ".mpkg"))
        return "application/vnd.apple.installer+xml";
    if (streq(file_ext, ".odp"))
        return "application/vnd.oasis.opendocument.presentation";
    if (streq(file_ext, ".ods"))
        return "application/vnd.oasis.opendocument.spreadsheet";
    if (streq(file_ext, ".odt"))
        return "application/vnd.oasis.opendocument.text";
    if (streq(file_ext, ".oga"))
        return "audio/ogg";
    if (streq(file_ext, ".ogv"))
        return "video/ogg";
    if (streq(file_ext, ".ogx"))
        return "application/ogg";
    if (streq(file_ext, ".opus"))
        return "audio/ogg";
    if (streq(file_ext, ".otf"))
        return "font/otf";
    if (streq(file_ext, ".png"))
        return "image/png";
    if (streq(file_ext, ".pdf"))
        return "application/pdf";
    if (streq(file_ext, ".php"))
        return "application/x-httpd-php";
    if (streq(file_ext, ".ppt"))
        return "application/vnd.ms-powerpoint";
    if (streq(file_ext, ".pptx"))
        return "application/"
               "vnd.openxmlformats-officedocument.presentationml.presentation";
    if (streq(file_ext, ".rar"))
        return "application/vnd.rar";
    if (streq(file_ext, ".rtf"))
        return "application/rtf";
    if (streq(file_ext, ".sh"))
        return "application/x-sh";
    if (streq(file_ext, ".svg"))
        return "image/svg+xml";
    if (streq(file_ext, ".tar"))
        return "application/x-tar";
    if (streq(file_ext, ".ts"))
        return "video/mp2t";
    if (streq(file_ext, ".ttf"))
        return "font/ttf";
    if (streq(file_ext, ".txt"))
        return "text/plain";
    if (streq(file_ext, ".vsd"))
        return "application/vnd.visio";
    if (streq(file_ext, ".wav"))
        return "audio/wav";
    if (streq(file_ext, ".weba"))
        return "audio/webm";
    if (streq(file_ext, ".webm"))
        return "video/webm";
    if (streq(file_ext, ".webmani"))
        return "festapplication/manifest+json";
    if (streq(file_ext, ".webp"))
        return "image/webp";
    if (streq(file_ext, ".woff"))
        return "font/woff";
    if (streq(file_ext, ".woff2"))
        return "font/woff2";
    if (streq(file_ext, ".xhtml"))
        return "application/xhtml+xml";
    if (streq(file_ext, ".xls"))
        return "application/vnd.ms-excel";
    if (streq(file_ext, ".xlsx"))
        return "application/"
               "vnd.openxmlformats-officedocument.spreadsheetml.sheet";
    if (streq(file_ext, ".xml"))
        return "application/xml";
    if (streq(file_ext, ".xul"))
        return "application/vnd.mozilla.xul+xml";
    if (streq(file_ext, ".zip"))
        return "application/zip";
    if (streq(file_ext, ".3gp"))
        return "video/3gpp";
    if (streq(file_ext, ".3g2"))
        return "video/3gpp2";
    if (streq(file_ext, ".7z"))
        return "application/x-7z-compressed";
    if (streq(file_ext, ".tif"))
        return "image/tiff";
    if (streq(file_ext, ".tiff"))
        return "image/tiff";
    if (streq(file_ext, ".htm"))
        return "text/html";
    if (streq(file_ext, ".html"))
        return "text/html";
    if (streq(file_ext, ".jpeg"))
        return "image/jpeg";
    if (streq(file_ext, ".jpg"))
        return "image/jpeg";
    if (streq(file_ext, ".mid"))
        return "audio/midi";
    if (streq(file_ext, ".midi"))
        return "audio/midi";
    return nullptr;
}

HTTP_Method parse_method(char* method_str) {
    if (!method_str)
        return HTTP_METHOD_ERR;

    if (streq(method_str, "GET"))
        return HTTP_GET;
    if (streq(method_str, "HEAD"))
        return HTTP_HEAD;
    else
        return HTTP_METHOD_ERR;
}

// parse the raw request path into a directory on the server.
HTTP_Target parse_target(char* req_path_raw) {
    char* res_filename = calloc(PATH_MAX, sizeof(char));

    if (!req_path_raw || streq(req_path_raw, "/")) {
        char index_html[] = "index.html";
        snprintf(res_filename, PATH_MAX, "%s/%s", document_root, index_html);
        return (HTTP_Target){
            .filename = res_filename,
            .mime_type = parse_mime_type(res_filename),
        };
    }
    strnlen
        // 1. Prepend the true document root
        char prepended_raw_path[PATH_MAX] = {};
    snprintf(prepended_raw_path, PATH_MAX, "%s%s", document_root, req_path_raw);

    // 2. resolve any symlinks, ../ or ./ in the path.
    if (!realpath(prepended_raw_path, res_filename)) {
        LOG_INFO("%s could not be resolved to a target.\n", prepended_raw_path);
        LOG_ERRNO();
        return NULL_TARGET;
    }
    // 3. ensure that after resolution, the request path still begins with the
    // document root (avoids the case of ../ escaping root dir)
    if (!cstr_startsWith(res_filename, document_root)) {
        LOG_INFO("%s does not begin with %s!\n", res_filename, document_root);
        return NULL_TARGET;
    }
    if (streq(res_filename, document_root)) {
        strcpy(res_filename, "/index.html");
    }
    LOG_DEBUG("Request resolved to: '%s'", res_filename);

    return (HTTP_Target){
        .filename = res_filename,
        .mime_type = parse_mime_type(res_filename),
    };
}

HTTP_Version parse_version(char* version_str) {
    if (!version_str)
        return HTTP_VER_ERR;

    if (streq(version_str, "1.1"))
        return HTTP_VER_1_1;
    return HTTP_VER_ERR;
}
