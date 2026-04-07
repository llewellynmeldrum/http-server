#pragma once
#include <string.h>
static inline bool streq(const char* s1, const char* s2) {
    return !strcmp(s1, s2);
}
static inline bool _isalpha(const char ch) {
    return (ch >= 'a' && ch <= 'z') || (ch >= 'A' && ch <= 'Z');
}
static inline bool _isdigit(const char ch) {
    return (ch >= '0' && ch <= '9');
}
static inline bool isalphanumeric(const char ch) {
    return _isalpha(ch) || _isdigit(ch);
}
static inline bool iswhitespace(const char ch) {
    return ch == ' ' || ch == '\t' || ch == '\r' || ch == '\n';
}
