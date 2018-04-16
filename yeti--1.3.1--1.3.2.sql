DROP FUNCTION tag_compare(a int [], b int[]);

CREATE OR REPLACE FUNCTION tag_compare(a int [], b int[], match_mode smallint = 0)
RETURNS int
AS '$libdir/yeti_pg_ext', 'tag_compare'
LANGUAGE C IMMUTABLE;
