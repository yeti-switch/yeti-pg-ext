#include "exported_functions.h"
#include "resolver.h"

#define LOG_PREFIX "lnp_set_rtt_timeout: "


PG_FUNCTION_INFO_V1(lnp_set_rtt_timeout);
Datum lnp_set_rtt_timeout(PG_FUNCTION_ARGS) {
	int32 t = PG_GETARG_INT32(0);
	Resolver.set_rtt_timeout(t);
	PG_RETURN_NULL();
}
