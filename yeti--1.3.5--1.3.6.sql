CREATE OR REPLACE FUNCTION tbf_rate_check(namespace_id INTEGER, bucket_id BIGINT, rate REAL)
RETURNS BOOLEAN
AS '$libdir/yeti_pg_ext', 'tbf_rate_check'
LANGUAGE C VOLATILE;
