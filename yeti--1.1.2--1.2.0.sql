CREATE TYPE lnp_result AS (lrn TEXT, tag TEXT);
CREATE OR REPLACE FUNCTION lnp_resolve_tagged(database_id INTEGER, local_number TEXT)
RETURNS lnp_result
AS '$libdir/yeti_pg_ext', 'lnp_resolve_tagged'
LANGUAGE C IMMUTABLE STRICT;
