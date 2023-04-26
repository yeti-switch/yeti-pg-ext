#include "request_id.h"
#include <stdlib.h>
#include <time.h>

uint32_t req_id;

uint32_t gen_request_id() {
	if (req_id == 0) {
		req_id = rand();
	} else {
		++req_id;
	}

	return req_id;
}
