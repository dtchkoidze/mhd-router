#include "../base/integers.h"
#include "../base/macros.h"
#include "router.h"
#include <assert.h>
#include <microhttpd.h>
#include "../string_view/string_view.h"

MHD_Result write_json(http_request *req, u16 status_code, s8 json) {
    if (req == NULL || json.len < 0 || (json.data == NULL && json.len != 0)) {
        return MHD_NO;
    }

    http_resp *resp = MHD_create_response_from_buffer((size_t)json.len, (void *)json.data, MHD_RESPMEM_MUST_COPY);
    if (resp == NULL) {
        return MHD_NO;
    }

    MHD_add_response_header(resp, "Content-Type", "Application/json");
    MHD_Result result = MHD_queue_response(req->conn, status_code, resp);
    MHD_destroy_response(resp);
    return result;
}
