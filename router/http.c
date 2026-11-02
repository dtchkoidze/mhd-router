#include "http.h"

#define DEFINE_HTTP_METHOD(name, text) \
    const char http_method_##name[] = text;

HTTP_METHOD_LIST(DEFINE_HTTP_METHOD)

#undef DEFINE_HTTP_METHOD
