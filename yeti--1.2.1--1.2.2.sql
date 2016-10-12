CREATE OR REPLACE FUNCTION rank_dns_srv(weight INTEGER)
RETURNS INTEGER
AS '$libdir/yeti_pg_ext', 'rank_dns_srv'
LANGUAGE C WINDOW;
