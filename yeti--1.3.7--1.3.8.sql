CREATE OR REPLACE FUNCTION lnp_endpoints_cache_set(key TEXT, response TEXT, error BOOLEAN)
RETURNS VOID
AS '$libdir/yeti_pg_ext', 'lnp_endpoints_cache_set'
LANGUAGE C IMMUTABLE STRICT;
