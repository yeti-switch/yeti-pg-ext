#pragma once

#include "postgres.h"
#include "fmgr.h"

Datum replace_rand(PG_FUNCTION_ARGS);
Datum regexp_replace_rand(PG_FUNCTION_ARGS);
Datum regexp_replace_rand_noopt(PG_FUNCTION_ARGS);
Datum regexp_replace_rand_array_noopt(PG_FUNCTION_ARGS);

Datum process_templates(PG_FUNCTION_ARGS);

Datum lnp_resolve_cnam(PG_FUNCTION_ARGS);
Datum lnp_resolve_tagged(PG_FUNCTION_ARGS);
Datum lnp_resolve_tagged_with_error(PG_FUNCTION_ARGS);

Datum lnp_endpoints_cache_set(PG_FUNCTION_ARGS);

Datum lnp_endpoints_set(PG_FUNCTION_ARGS);
Datum lnp_endpoints_show(PG_FUNCTION_ARGS);

Datum lnp_set_timeout(PG_FUNCTION_ARGS);
Datum lnp_set_rtt_timeout(PG_FUNCTION_ARGS);

Datum rank_dns_srv(PG_FUNCTION_ARGS);

Datum tag_action(PG_FUNCTION_ARGS);
Datum tag_compare(PG_FUNCTION_ARGS);

Datum tbf_rate_check(PG_FUNCTION_ARGS);

void _PG_init(void);
void _PG_fini(void);
