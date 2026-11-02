#pragma once

#define HTTP_METHOD_LIST(X) \
    X(get,     "GET")       \
    X(head,    "HEAD")      \
    X(post,    "POST")      \
    X(put,     "PUT")       \
    X(patch,   "PATCH")     \
    X(delete,  "DELETE")    \
    X(options, "OPTIONS")

#define DECLARE_HTTP_METHOD(name, text) \
    extern const char http_method_##name[];

HTTP_METHOD_LIST(DECLARE_HTTP_METHOD)

#undef DECLARE_HTTP_METHOD
