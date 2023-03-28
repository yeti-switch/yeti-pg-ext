#pragma once

#include <stdlib.h>
#include <stdbool.h>

#include "postgres.h"

struct endpoints_cache {
	int (*store)(const text *key, const text *response, bool error);
	int (*find)(const text *, char **response_out, bool *error_out);
	int (*items_count)(void);
	int (*remove)(const text *key);
	int (*remove_all)(void);
	int (*destroy)(void);
};

extern const struct endpoints_cache EndpointsCache;
