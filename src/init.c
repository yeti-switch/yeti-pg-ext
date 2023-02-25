#include "exported_functions.h"
#include "log.h"
#include "transport.h"
#include "f_tbf_rate_check.h"

#define LOG_PREFIX ""

void _PG_init(void)
{
	Transport.init();
	tbf_init();
}
