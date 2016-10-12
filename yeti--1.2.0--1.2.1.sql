CREATE OR REPLACE FUNCTION regexp_replace_rand(TEXT,TEXT,TEXT,TEXT)
RETURNS TEXT
AS '$libdir/yeti_pg_ext', 'regexp_replace_rand'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION regexp_replace_rand(TEXT,TEXT,TEXT)
RETURNS TEXT
AS '$libdir/yeti_pg_ext', 'regexp_replace_rand_noopt'
LANGUAGE C IMMUTABLE;
