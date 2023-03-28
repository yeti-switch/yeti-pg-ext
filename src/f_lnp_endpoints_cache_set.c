#include "exported_functions.h"
#include "log.h"
#include "endpoints_cache.h"

#define LOG_PREFIX "lnp_endpoints_cache: "

PG_FUNCTION_INFO_V1(lnp_endpoints_cache_set);
Datum lnp_endpoints_cache_set(PG_FUNCTION_ARGS)
{
	EndpointsCache.store(
		PG_GETARG_TEXT_P(0),
		PG_GETARG_TEXT_P(1),
		PG_GETARG_BOOL(2));

	PG_RETURN_NULL();
}
