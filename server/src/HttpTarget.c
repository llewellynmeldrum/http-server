#include "HttpTarget.h"
#include "StringMap.h"
#include <limits.h>
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
static String resolve_HttpTarget(StringView raw_target);
static String resolve_absoluteForm(StringView raw_target);
static String normalize_Path(String path);

HttpTarget resolve_HttpRequest(HttpRequest request) {
    HttpTarget res = {};
    if (request.hasError) {
        res.resolved_path = NULL_STRING;
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
        // escapes docroot?
        // TODO: handle error
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

static String resolve_absoluteForm(StringView raw_target) {
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

static String resolve_originForm(StringView raw_target) {
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

static String decode_percentEscapes(String path) {
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

    // Stack based approach, push path segments, pop if '..'. If we reach an empty stack, we have
    // tried to escape docroot
    // I realise that this is all kind of unnecessary because realpath() does this anyway.
    // But whatever.
    StringView* stack[BUF_SZ] = {};
    size_t      st_top = 0;
    LOG_EXPR(num_segments);
    for (size_t i = 0; i < num_segments; i++) {
        //        LOG_EXPR(path_segments[i]);
        if (sv_equal(path_segments[i], EMPTY) || sv_equal(path_segments[i], DOT)) {
            continue;
        } else if (sv_equal(path_segments[i], DOTDOT)) {
            if (st_top <= 0) {
                return NULL_STRING;  // stack is empty, tried to escape docroot
            } else {
                st_top--;
                //          LOG_DEBUG("POP:");
                //         LOG_EXPR(path_segments[i]);
            }
        } else {
            //    LOG_DEBUG("PUSH:");
            //   LOG_EXPR(path_segments[i]);
            stack[st_top++] = &path_segments[i];
        }
    }

    String res = str_make_empty();
    for (size_t i = 0; i < st_top; i++) {
        str_append_sv(&res, FSLASH);
        str_append_sv(&res, *stack[i]);
    }
    return res;
}
static String normalize_Path(String path) {
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

static String resolve_HttpTarget(StringView raw_target) {
    //    find out if absolute_form or origin_form;
    typedef enum {
        HttpTargetType_ABSOLUTE,
        HttpTargetType_ORIGIN,
        HttpTargetType_BAD_FORM,
    } HttpTargetType;
    HttpTargetType req_type;
    if (sv_hasPrefixStr(raw_target, "http://")) {
        req_type = HttpTargetType_ABSOLUTE;
    } else if (sv_at(raw_target, 0) == '/') {
        req_type = HttpTargetType_ORIGIN;
    } else {
        req_type = HttpTargetType_BAD_FORM;
    }

    switch (req_type) {
    case HttpTargetType_ABSOLUTE:
        return resolve_absoluteForm(raw_target);
        break;
    case HttpTargetType_ORIGIN:
        return resolve_originForm(raw_target);
        break;
    case HttpTargetType_BAD_FORM:
        return NULL_STRING;
        break;
    }
}
void init_percent_valmap(void) {
    percent_encoding_map = sm_make();
    sm_mapArrays(&percent_encoding_map, percent_keymap, percent_valmap, 20);
    //    sm_print(&percent_encoding_map);
}
