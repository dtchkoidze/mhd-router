#include "../router/router.h"
#include "../router/util.h"
#include <assert.h>
#include <libpq-fe.h>
#include <microhttpd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../string_view/string_view.h"

/**
 *
 *
 * WHOLE TING IS DEMO + WIP
 */

MHD_Result handle_get_health(http_request *req) {
    assert(strcmp(req->method, "GET") == 0);
    s8 json = s8_lit("{\"status\": \"operational\"}");
    return write_json(req, MHD_HTTP_OK, json);
}

MHD_Result handle_get_parameterized(http_request *req) {
    assert(strcmp(req->method, "GET") == 0);
    const http_url_param *id = router_url_param(req, "id");
    if (id == NULL) {
        return MHD_NO;
    }

    char json[256];
    int written = snprintf(
        json,
        sizeof(json),
        "{\"user\":\"%.*s\"}",
        (int)id->value_length,
        id->value
    );

    if (written < 0 || (size_t)written >= sizeof(json)) {
        return MHD_NO;
    }

    MHD_Result ret;
    struct MHD_Response *res;

    res = MHD_create_response_from_buffer(written, json, MHD_RESPMEM_PERSISTENT);
    MHD_add_response_header(res, "Content-Type", "Application/json");
    ret = MHD_queue_response(req->conn, MHD_HTTP_OK, res);
    MHD_destroy_response(res);
    return ret;
}


MHD_Result handle_get_test(http_request *req) {
    assert(strcmp(req->method, "GET") == 0);
    s8 json = s8_lit("{\"testing\": \"yuuurrr\"}");
    return write_json(req, MHD_HTTP_OK, json);
}

MHD_Result handle_get_tekla(http_request *req) {
    assert(strcmp(req->method, "GET") == 0);
    s8 json = s8_lit("{\"endpoint\": \"tekla\"}");
    return write_json(req, MHD_HTTP_OK, json);
}

int main(int argc, char **argv) {
    struct MHD_Daemon *d;

    if (argc != 2) {
        printf("%s PORT\n", argv[0]);
        return 1;
    }

    // testing s8s
    s8 str = s8_lit("AYOOO LITERAL FUCKING STRING");
    printf("my striiing: %.*s\n", s8_spread(str));

    u8 compact_data[128];
    s8 compact = s8_trimspace(&str, compact_data, sizeof(compact_data));
    printf("without whitespace: %.*s\n", s8_spread(compact));


    http_router *r = &(http_router){0};
    r_result r_init = router_init(r, "/api");
    if (!r_init.ok) {
        printf("initialization of router failed, reason: %s\n", r_init.message);
        return 1;
    }

    router_get(r, "/test", handle_get_test);
    router_get(r, "/tekla", handle_get_tekla);
    router_get(r, "/health", handle_get_health);
    router_get(r, "/users/:id", handle_get_parameterized);

    #ifdef DEBUG
        router_print(r);
    #endif

    printf("starting http server at: %s\n", argv[1]);
    d = router_serve(r,
                     MHD_USE_THREAD_PER_CONNECTION | MHD_USE_INTERNAL_POLLING_THREAD,
                     atoi(argv[1]));

    if (NULL == d) {
        printf("server failed...\n");
        return 1;
    }

    (void)getc(stdin);

    MHD_stop_daemon(d);
    router_destroy(r);

    return 0;
}
