DROP FUNCTION lnp_resolve(INTEGER, TEXT);

CREATE OR REPLACE FUNCTION lnp_resolve_cnam(database_id INTEGER, data JSON)
RETURNS JSON
AS '$libdir/yeti_pg_ext', 'lnp_resolve_cnam'
LANGUAGE C IMMUTABLE STRICT;
