#pragma once

#include <microhttpd.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "rax.h"

#include "http.h"

// MHD wrapper types
typedef enum MHD_Result MHD_Result;
typedef struct MHD_Connection http_conn;
typedef struct MHD_Response http_resp;
// END


typedef struct http_url_param {
    const char *key;
    const char *value;
    size_t value_length;
} http_url_param;

typedef struct http_request {
    http_conn *conn;
    const char *url;
    const char *method;
    const char *version;
    const char *upload_data;
    size_t *upload_data_size;
    const http_url_param *url_params;
    size_t url_param_count;
} http_request;

typedef MHD_Result (*handler_func)(http_request *);

typedef struct route_node route_node;
typedef struct http_router {
    char *prefix;
    route_node *root;
} http_router;

typedef struct r_result {
    const char *message; // later gotta move to s8 string views
    bool ok;
} r_result;

typedef struct endpoint {
    handler_func handler;
} endpoint;

struct route_node {
    rax *static_children;
    route_node *parameter_child;
    char *parameter_name;
    rax *methods;
};

typedef struct path_segment {
    const char *data;
    size_t length;
} path_segment;

typedef struct path_segments {
    path_segment *items;
    size_t count;
} path_segments;

r_result router_init(http_router *router, const char *prefix);
r_result router_destroy(http_router *router);
r_result router_add(http_router *router, const char *method, const char *path,
                    handler_func handler);
void router_print(http_router *router);
struct MHD_Daemon *router_serve(http_router *router, unsigned int flags, uint16_t port);
MHD_Result router_dispatch(http_router *router, http_request *request);

// The returned value points into the request URL and remains valid only while
// the route handler is running. It is not NUL-terminated; use value_length.
const http_url_param *router_url_param(const http_request *request, const char *key);

#define DEFINE_ROUTER_METHOD(name, text)                                        \
    static inline r_result router_##name(http_router *router, const char *path, \
                                          handler_func handler)                 \
    {                                                                           \
        return router_add(router, http_method_##name, path, handler);           \
    }

HTTP_METHOD_LIST(DEFINE_ROUTER_METHOD)

#undef DEFINE_ROUTER_METHOD
