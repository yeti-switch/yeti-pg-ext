CREATE OR REPLACE FUNCTION tag_action(op int, a int [], b int[])
RETURNS int[]
AS '$libdir/yeti_pg_ext', 'tag_action'
LANGUAGE C IMMUTABLE STRICT;

CREATE OR REPLACE FUNCTION tag_compare(a int [], b int[])
RETURNS int
AS '$libdir/yeti_pg_ext', 'tag_compare'
LANGUAGE C IMMUTABLE;
