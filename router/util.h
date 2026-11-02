#include "router.h"
#include "../base/integers.h"
#include "../string_view/string_view.h"

MHD_Result write_json(http_request *req, u16 status_code, s8 json);
