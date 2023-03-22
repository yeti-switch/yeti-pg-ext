CREATE TYPE lnp_result_with_error AS (lrn TEXT, tag TEXT, error TEXT);
CREATE OR REPLACE FUNCTION lnp_resolve_tagged_with_error(database_id INTEGER, local_number TEXT)
RETURNS lnp_result_with_error
AS '$libdir/yeti_pg_ext', 'lnp_resolve_tagged_with_error'
LANGUAGE C IMMUTABLE STRICT;
