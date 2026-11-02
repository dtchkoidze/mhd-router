#include "../base/integers.h"
#include "../base/macros.h"
#include "router.h"
#include <assert.h>
#include <microhttpd.h>


// TODO should take u8
MHD_Result write_json(http_request *req, u16 status_code, http_resp *resp) {
    assert(req != NULL);
    MHD_add_response_header(resp, "Content-Type", "Application/json");
    MHD_Result result = MHD_queue_response(req->conn, status_code, resp);
    MHD_destroy_response(resp);
    return result;
}
