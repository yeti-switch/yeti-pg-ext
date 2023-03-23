#include "exported_functions.h"
#include "log.h"
#include "endpoints_cache.h"
#include "utils.h"

#define LOG_PREFIX "lnp_endpoints_cache: "

PG_FUNCTION_INFO_V1(lnp_endpoints_cache_set);
Datum lnp_endpoints_cache_set(PG_FUNCTION_ARGS) {
	char *key, *response;
	bool error;

	key = STR_FROM_TEXTARG(0);
	response = STR_FROM_TEXTARG(1);
	error = PG_GETARG_BOOL(2);

	EndpointsCache.store(key, response, error);

	pfree(key); key = NULL;
	pfree(response); response = NULL;

	PG_RETURN_NULL();
}
