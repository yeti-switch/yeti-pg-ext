#include "exported_functions.h"
#include "log.h"
#include "transport.h"
#include "endpoints_cache.h"
#include "f_tbf_rate_check.h"

#define LOG_PREFIX ""

void _PG_fini(void)
{
	Transport.shutdown_socket();
	EndpointsCache.destroy();
	tbf_fini();
}
