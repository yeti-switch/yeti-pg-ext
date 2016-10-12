---
--- import functions
---
CREATE OR REPLACE FUNCTION replace_rand(TEXT)
RETURNS TEXT
AS '$libdir/yeti_pg_ext', 'replace_rand'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION regexp_replace_rand(TEXT,TEXT,TEXT,TEXT)
RETURNS TEXT
AS '$libdir/yeti_pg_ext', 'regexp_replace_rand'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION regexp_replace_rand(TEXT,TEXT,TEXT)
RETURNS TEXT
AS '$libdir/yeti_pg_ext', 'regexp_replace_rand_noopt'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION lnp_resolve(database_id INTEGER, local_number TEXT)
RETURNS TEXT
AS '$libdir/yeti_pg_ext', 'lnp_resolve'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE lnp_result AS (lrn TEXT, tag TEXT);
CREATE OR REPLACE FUNCTION lnp_resolve_tagged(database_id INTEGER, local_number TEXT)
RETURNS lnp_result
AS '$libdir/yeti_pg_ext', 'lnp_resolve_tagged'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION lnp_endpoints_set(endpoints TEXT[])
RETURNS VOID
AS '$libdir/yeti_pg_ext', 'lnp_endpoints_set'
LANGUAGE C IMMUTABLE STRICT;

CREATE TYPE endpoints AS (id integer, uri TEXT);
CREATE OR REPLACE FUNCTION lnp_endpoints_show()
RETURNS SETOF endpoints
AS '$libdir/yeti_pg_ext', 'lnp_endpoints_show'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION lnp_set_timeout(timeout_msec INTEGER)
RETURNS VOID
AS '$libdir/yeti_pg_ext', 'lnp_set_timeout'
LANGUAGE C IMMUTABLE STRICT;
