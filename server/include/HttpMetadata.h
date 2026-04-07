#pragma once
// clang-format off
    #define SET_ERROR(res, status)\
    LOG_ERROR("%s",#status);\
    res.hasError=true;\
    res.err = (HttpError){\
        .code = HttpStatus_##status,\
        .src = (ErrorSource){\
            .line= __LINE__,\
            .name= __FILE_NAME__,\
        },\
    };
typedef enum {
    HttpStatus_CONTINUE                           = 100,
    HttpStatus_SWITCHING_PROTOCOLS                = 101,
    HttpStatus_PROCESSING                         = 102,
    HttpStatus_EARLY_HINTS                        = 103,
    HttpStatus_UPLOAD_RESUMPTION_SUPPORTED        = 104,
    HttpStatus_OK                                 = 200,
    HttpStatus_CREATED                            = 201,
    HttpStatus_ACCEPTED                           = 202,
    HttpStatus_NON_AUTHORITATIVE_INFORMATION      = 203,
    HttpStatus_NO_CONTENT                         = 204,
    HttpStatus_RESET_CONTENT                      = 205,
    HttpStatus_PARTIAL_CONTENT                    = 206,
    HttpStatus_MULTI_STATUS                       = 207,
    HttpStatus_ALREADY_REPORTED                   = 208,
    HttpStatus_IM_USED                            = 226,
    HttpStatus_MULTIPLE_CHOICES                   = 300,
    HttpStatus_MOVED_PERMANENTLY                  = 301,
    HttpStatus_FOUND                              = 302,
    HttpStatus_SEE_OTHER                          = 303,
    HttpStatus_NOT_MODIFIED                       = 304,
    HttpStatus_USE_PROXY                          = 305,
    HttpStatus_UNUSED                             = 306,
    HttpStatus_TEMPORARY_REDIRECT                 = 307,
    HttpStatus_PERMANENT_REDIRECT                 = 308,
    HttpStatus_BAD_REQUEST                        = 400,
    HttpStatus_UNAUTHORIZED                       = 401,
    HttpStatus_PAYMENT_REQUIRED                   = 402,
    HttpStatus_FORBIDDEN                          = 403,
    HttpStatus_NOT_FOUND                          = 404,
    HttpStatus_METHOD_NOT_ALLOWED                 = 405,
    HttpStatus_NOT_ACCEPTABLE                     = 406,
    HttpStatus_PROXY_AUTHENTICATION_REQUIRED      = 407,
    HttpStatus_REQUEST_TIMEOUT                    = 408,
    HttpStatus_CONFLICT                           = 409,
    HttpStatus_GONE                               = 410,
    HttpStatus_LENGTH_REQUIRED                    = 411,
    HttpStatus_PRECONDITION_FAILED                = 412,
    HttpStatus_CONTENT_TOO_LARGE                  = 413,
    HttpStatus_URI_TOO_LONG                       = 414,
    HttpStatus_UNSUPPORTED_MEDIA_TYPE             = 415,
    HttpStatus_RANGE_NOT_SATISFIABLE              = 416,
    HttpStatus_EXPECTATION_FAILED                 = 417,
    HttpStatus_IM_A_TEAPOT                        = 418,
    HttpStatus_MISDIRECTED_REQUEST                = 421,
    HttpStatus_UNPROCESSABLE_CONTENT              = 422,
    HttpStatus_LOCKED                             = 423,
    HttpStatus_FAILED_DEPENDENCY                  = 424,
    HttpStatus_TOO_EARLY                          = 425,
    HttpStatus_UPGRADE_REQUIRED                   = 426,
    HttpStatus_UNASSIGNED_1                       = 427,
    HttpStatus_PRECONDITION_REQUIRED              = 428,
    HttpStatus_TOO_MANY_REQUESTS                  = 429,
    HttpStatus_UNASSIGNED_2                       = 430,
    HttpStatus_REQUEST_HEADER_FIELDS_TOO_LARGE    = 431,
    HttpStatus_UNAVAILABLE_FOR_LEGAL_REASONS      = 451,
    HttpStatus_INTERNAL_SERVER_ERROR              = 500,
    HttpStatus_NOT_IMPLEMENTED                    = 501,
    HttpStatus_BAD_GATEWAY                        = 502,
    HttpStatus_SERVICE_UNAVAILABLE                = 503,
    HttpStatus_GATEWAY_TIMEOUT                    = 504,
    HttpStatus_HTTP_VERSION_NOT_SUPPORTED         = 505,
    HttpStatus_VARIANT_ALSO_NEGOTIATES            = 506,
    HttpStatus_INSUFFICIENT_STORAGE               = 507,
    HttpStatus_LOOP_DETECTED                      = 508,
    HttpStatus_UNASSIGNED_3                       = 509,
    HttpStatus_NOT_EXTENDED                       = 510,
    HttpStatus_NETWORK_AUTHENTICATION_REQUIRED    = 511,
}HttpStatus;


