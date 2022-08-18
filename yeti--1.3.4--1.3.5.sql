CREATE OR REPLACE FUNCTION regexp_replace_rand(text_in TEXT[], regexp_rule TEXT, regexp_result TEXT, keep_empty BOOL = FALSE)
RETURNS TEXT[]
AS '$libdir/yeti_pg_ext', 'regexp_replace_rand_array_noopt'
LANGUAGE C VOLATILE;
