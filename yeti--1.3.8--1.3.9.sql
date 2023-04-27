CREATE OR REPLACE FUNCTION lnp_set_rtt_timeout(timeout_msec INTEGER)
RETURNS VOID
AS '$libdir/yeti_pg_ext', 'lnp_set_rtt_timeout'
LANGUAGE C IMMUTABLE STRICT;
