#pragma once

#include <stdlib.h>
#include <stdbool.h>

struct endpoints_cache {
	int (*store)(const char *key, const char *response, bool error);
	int (*find)(const char *key, char **response_out, bool *error_out);
	int (*items_count)(void);
	int (*remove)(const char *key);
	int (*remove_all)(void);
	int (*destroy)(void);
};

extern const struct endpoints_cache EndpointsCache;
