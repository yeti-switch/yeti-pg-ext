CREATE OR REPLACE FUNCTION process_templates(template text, vars jsonb)
RETURNS text
AS '$libdir/yeti_pg_ext', 'process_templates'
LANGUAGE C IMMUTABLE;

CREATE OR REPLACE FUNCTION process_templates(templates text[], vars jsonb)
RETURNS text[]
AS '$libdir/yeti_pg_ext', 'process_templates'
LANGUAGE C IMMUTABLE;
