#include "router.h"

#include "../base/macros.h"
#include "rax.h"

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static void route_node_destroy(void *data);

static char *copy_string(const char *string) {
    size_t length = strlen(string);
    char *copy = malloc(length + 1);
    if (copy != NULL) {
        memcpy(copy, string, length + 1);
    }
    return copy;
}

static char *copy_string_view(const char *string, size_t length) {
    if (length == SIZE_MAX) {
        return NULL;
    }

    char *copy = malloc(length + 1);
    if (copy != NULL) {
        memcpy(copy, string, length);
        copy[length] = '\0';
    }
    return copy;
}

static route_node *route_node_create(void) {
    route_node *node = calloc(1, sizeof(*node));
    if (node == NULL) {
        return NULL;
    }

    node->static_children = raxNew();
    node->methods = raxNew();
    if (node->static_children == NULL || node->methods == NULL) {
        if (node->static_children != NULL) {
            raxFree(node->static_children);
        }
        if (node->methods != NULL) {
            raxFree(node->methods);
        }
        free(node);
        return NULL;
    }

    return node;
}

static void route_node_destroy(void *data) {
    route_node *node = data;
    if (node == NULL) {
        return;
    }

    raxFreeWithCallback(node->static_children, route_node_destroy);
    route_node_destroy(node->parameter_child);
    raxFreeWithCallback(node->methods, free);
    free(node->parameter_name);
    free(node);
}

static bool path_segments_create(const char *path, path_segments *segments) {
    if (path == NULL || segments == NULL) {
        return false;
    }

    *segments = (path_segments){0};

    size_t count = 0;
    const char *cursor = path;
    while (*cursor != '\0') {
        while (*cursor == '/') {
            cursor += 1;
        }
        if (*cursor == '\0') {
            break;
        }

        count += 1;
        while (*cursor != '\0' && *cursor != '/') {
            cursor += 1;
        }
    }

    if (count == 0) {
        return true;
    }
    if (count > SIZE_MAX / sizeof(*segments->items)) {
        return false;
    }

    segments->items = malloc(count * sizeof(*segments->items));
    if (segments->items == NULL) {
        return false;
    }

    cursor = path;
    size_t index = 0;
    while (*cursor != '\0') {
        while (*cursor == '/') {
            cursor += 1;
        }
        if (*cursor == '\0') {
            break;
        }

        const char *start = cursor;
        while (*cursor != '\0' && *cursor != '/') {
            cursor += 1;
        }

        segments->items[index++] = (path_segment){
            .data = start,
            .length = (size_t)(cursor - start),
        };
    }

    segments->count = index;
    return true;
}

static void path_segments_destroy(path_segments *segments) {
    free(segments->items);
    *segments = (path_segments){0};
}

static route_node *route_node_static_child(route_node *node, path_segment segment) {
    void *data = raxFind(node->static_children, (unsigned char *)segment.data, segment.length);
    return data == raxNotFound ? NULL : data;
}

static route_node *route_node_get_or_create_static_child(route_node *node, path_segment segment) {
    route_node *child = route_node_static_child(node, segment);
    if (child != NULL) {
        return child;
    }

    child = route_node_create();
    if (child == NULL) {
        return NULL;
    }

    errno = 0;
    if (!raxTryInsert(node->static_children, (unsigned char *)segment.data, segment.length, child,
                      NULL)) {
        route_node_destroy(child);
        return NULL;
    }

    return child;
}

static route_node *route_node_get_or_create_parameter_child(route_node *node, path_segment segment,
                                                            const char **error) {
    const char *name = segment.data + 1;
    size_t name_length = segment.length - 1;

    if (node->parameter_child != NULL) {
        if (strlen(node->parameter_child->parameter_name) != name_length ||
            memcmp(node->parameter_child->parameter_name, name, name_length) != 0) {
            *error = "conflicting parameter name at the same route position";
            return NULL;
        }
        return node->parameter_child;
    }

    route_node *child = route_node_create();
    if (child == NULL) {
        *error = "failed to allocate parameter route node";
        return NULL;
    }

    child->parameter_name = copy_string_view(name, name_length);
    if (child->parameter_name == NULL) {
        route_node_destroy(child);
        *error = "failed to allocate parameter name";
        return NULL;
    }

    node->parameter_child = child;
    return child;
}

static route_node *route_insert_path(route_node *node, const char *path, const char **error) {
    path_segments segments;
    if (!path_segments_create(path, &segments)) {
        *error = "failed to split route path";
        return NULL;
    }

    for (size_t index = 0; index < segments.count; index += 1) {
        path_segment segment = segments.items[index];

        if (segment.data[0] == ':') {
            if (segment.length == 1) {
                *error = "route parameter name cannot be empty";
                node = NULL;
                break;
            }
            node = route_node_get_or_create_parameter_child(node, segment, error);
        } else {
            node = route_node_get_or_create_static_child(node, segment);
            if (node == NULL) {
                *error = "failed to allocate static route node";
            }
        }

        if (node == NULL) {
            break;
        }
    }

    path_segments_destroy(&segments);
    return node;
}

static endpoint *route_node_endpoint(route_node *node, const char *method) {
    void *data = raxFind(node->methods, (unsigned char *)method, strlen(method));
    return data == raxNotFound ? NULL : data;
}

static endpoint *route_match(route_node *node, const char *method, const path_segments *segments,
                             size_t segment_index, http_url_param *params, size_t *param_count) {
    if (segment_index == segments->count) {
        return route_node_endpoint(node, method);
    }

    path_segment segment = segments->items[segment_index];
    size_t original_param_count = *param_count;

    route_node *static_child = route_node_static_child(node, segment);
    if (static_child != NULL) {
        endpoint *found =
            route_match(static_child, method, segments, segment_index + 1, params, param_count);
        if (found != NULL) {
            return found;
        }
        *param_count = original_param_count;
    }

    if (node->parameter_child != NULL) {
        params[*param_count] = (http_url_param){
            .key = node->parameter_child->parameter_name,
            .value = segment.data,
            .value_length = segment.length,
        };
        *param_count += 1;

        endpoint *found = route_match(node->parameter_child, method, segments, segment_index + 1,
                                      params, param_count);
        if (found != NULL) {
            return found;
        }
        *param_count = original_param_count;
    }

    return NULL;
}

static MHD_Result router_send_not_found(http_request *request) {
    static const char body[] = "{\"error\":\"not found\"}";
    struct MHD_Response *response =
        MHD_create_response_from_buffer(sizeof(body) - 1, (void *)body, MHD_RESPMEM_PERSISTENT);
    if (response == NULL) {
        return MHD_NO;
    }

    MHD_add_response_header(response, "Content-Type", "application/json");
    MHD_Result result = MHD_queue_response(request->conn, MHD_HTTP_NOT_FOUND, response);
    MHD_destroy_response(response);
    return result;
}

static MHD_Result mhd_handler(void *cls, struct MHD_Connection *connection, const char *url,
                              const char *method, const char *version, const char *upload_data,
                              size_t *upload_data_size, void **request_context) {
    (void)request_context;

    http_request request = {
        .conn = connection,
        .url = url,
        .method = method,
        .upload_data = upload_data,
        .upload_data_size = upload_data_size,
        .version = version,
    };

    return router_dispatch(cls, &request);
}

const http_url_param *router_url_param(const http_request *request, const char *key) {
    if (request == NULL || key == NULL) {
        return NULL;
    }

    for (size_t index = 0; index < request->url_param_count; index += 1) {
        if (strcmp(request->url_params[index].key, key) == 0) {
            return &request->url_params[index];
        }
    }

    return NULL;
}

MHD_Result router_dispatch(http_router *router, http_request *request) {
    if (router == NULL || router->root == NULL || request == NULL || request->method == NULL ||
        request->url == NULL) {
        return MHD_NO;
    }

    path_segments segments;
    if (!path_segments_create(request->url, &segments)) {
        return MHD_NO;
    }

    http_url_param *params = NULL;
    if (segments.count > 0) {
        params = calloc(segments.count, sizeof(*params));
        if (params == NULL) {
            path_segments_destroy(&segments);
            return MHD_NO;
        }
    }

    size_t param_count = 0;
    endpoint *found =
        route_match(router->root, request->method, &segments, 0, params, &param_count);

    if (found == NULL) {
        debugf("no handler found: method=%s, path=%s\n", request->method, request->url);
        free(params);
        path_segments_destroy(&segments);
        return router_send_not_found(request);
    }

    request->url_params = params;
    request->url_param_count = param_count;
    MHD_Result result = found->handler(request);
    request->url_params = NULL;
    request->url_param_count = 0;

    free(params);
    path_segments_destroy(&segments);
    return result;
}

r_result router_add(http_router *router, const char *method, const char *path,
                    handler_func handler) {
    r_result result = {0};
    if (router == NULL || router->root == NULL || method == NULL || method[0] == '\0' ||
        path == NULL || handler == NULL) {
        result.message = "invalid route registration";
        return result;
    }

    debugf("adding endpoint to router: method=%s, prefix=%s, path=%s\n", method, router->prefix,
           path);

    const char *error = NULL;
    route_node *terminal = route_insert_path(router->root, router->prefix, &error);
    if (terminal != NULL) {
        terminal = route_insert_path(terminal, path, &error);
    }
    if (terminal == NULL) {
        result.message = error;
        return result;
    }

    if (route_node_endpoint(terminal, method) != NULL) {
        result.message = "route already exists";
        return result;
    }

    endpoint *new_endpoint = malloc(sizeof(*new_endpoint));
    if (new_endpoint == NULL) {
        result.message = "failed to allocate endpoint";
        return result;
    }
    new_endpoint->handler = handler;

    errno = 0;
    if (!raxTryInsert(terminal->methods, (unsigned char *)method, strlen(method), new_endpoint,
                      NULL)) {
        free(new_endpoint);
        result.message =
            errno == ENOMEM ? "failed to allocate method node" : "route already exists";
        return result;
    }

    result.ok = true;
    return result;
}

static void print_indent(size_t depth) {
    for (size_t index = 0; index < depth; index += 1) {
        fputs("  ", stdout);
    }
}

static void route_node_print(route_node *node, size_t depth) {
    raxIterator methods;
    raxStart(&methods, node->methods);
    raxSeek(&methods, "^", NULL, 0);
    while (raxNext(&methods)) {
        print_indent(depth);
        printf("[%.*s]\n", (int)methods.key_len, (char *)methods.key);
    }
    raxStop(&methods);

    raxIterator children;
    raxStart(&children, node->static_children);
    raxSeek(&children, "^", NULL, 0);
    while (raxNext(&children)) {
        print_indent(depth);
        printf("/%.*s\n", (int)children.key_len, (char *)children.key);
        route_node_print(children.data, depth + 1);
    }
    raxStop(&children);

    if (node->parameter_child != NULL) {
        print_indent(depth);
        printf("/:%s\n", node->parameter_child->parameter_name);
        route_node_print(node->parameter_child, depth + 1);
    }
}

void router_print(http_router *router) {
    if (router == NULL || router->root == NULL) {
        printf("http_router is nil\n");
        return;
    }

    printf("router prefix=%s\n", router->prefix);
    route_node_print(router->root, 0);
}

r_result router_init(http_router *router, const char *prefix) {
    r_result result = {0};
    if (router == NULL) {
        result.message = "router is null";
        return result;
    }

    debugf("initializing router...\n");
    if (prefix == NULL) {
        prefix = "";
    }

    router->prefix = copy_string(prefix);
    router->root = route_node_create();
    if (router->prefix == NULL || router->root == NULL) {
        free(router->prefix);
        route_node_destroy(router->root);
        *router = (http_router){0};
        result.message = "failed to allocate router";
        return result;
    }

    result.ok = true;
    return result;
}

r_result router_destroy(http_router *router) {
    r_result result = {0};
    if (router == NULL) {
        result.message = "router is null";
        return result;
    }

    route_node_destroy(router->root);
    free(router->prefix);
    *router = (http_router){0};
    result.ok = true;
    return result;
}

struct MHD_Daemon *router_serve(http_router *router, unsigned int flags, uint16_t port) {
    if (router == NULL || router->root == NULL) {
        return NULL;
    }

    return MHD_start_daemon(flags, port, NULL, NULL, mhd_handler, router, MHD_OPTION_END);
}
