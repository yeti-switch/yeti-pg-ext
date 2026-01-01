-- tests are written to meet expected values tables here: https://yeti-switch.org/documentation/best-practices/using-routing-tags.html
BEGIN;

-- CREATE EXTENSION IF NOT EXISTS yeti WITH SCHEMA yeti_ext;
set search_path TO public,yeti_ext;

-- Load the TAP functions.
-- TODO: find the way to include pgtap.sql for the related major version (:SERVER_VERSION_NUM/10000)
\i /usr/share/postgresql/18/extension/pgtap.sql

-- process_templates(templates text[], vars jsonb) RETURNS text[]
CREATE FUNCTION test_replace_placeholders_textarray() RETURNS SETOF TEXT AS $$ BEGIN

    RETURN NEXT is(process_templates(NULL, '{}'), NULL, 'NULL templates. return NULL');
    RETURN NEXT is(process_templates(array['qwe'], '{}'), array['qwe'], 'no placeholders. return input');
    RETURN NEXT is(process_templates(array['qwe'], NULL), array['qwe'], 'NULL vars. return input');
    RETURN NEXT is(process_templates(array[]::text[], '{}'), array[]::text[], 'empty array input. return empty array');

    RETURN NEXT is(process_templates(array['}}qwe{{rty{{'], '{}'), array['}}qwerty{{'], 'unbalanced placeholders. keep closing. remove first opening');

    RETURN NEXT is(process_templates(array['q{{nx}}e'], '{"w":42}'), array['qe'], 'wrong placeholder name');
    RETURN NEXT is(process_templates(array['q{{vars.}}e'], '{"w":42}'), array['qe'], 'incomplete placeholder name');
    RETURN NEXT is(process_templates(array['q{{vars.nx}}e'], '{"w":42}'), array['qnulle'], 'nx placeholder name');

    RETURN NEXT is(process_templates(array['q{{vars.w}}e'], '{"w":42}'), array['q42e'], 'single replacement');

    RETURN NEXT is(process_templates(array['q{{vars.w}}e'], '{"w":[42]}'), array['qe'], 'non-scalar replacement');

    RETURN NEXT is(
        process_templates(
            array[
                'i:{{vars.i}},f:{{vars.f}},sci:{{vars.sci}}',
                NULL,
                's:{{vars.s}}',
                '{{vars.true}} is not {{vars.false}}'
            ],
            '{ "i":42, "f":"42.0", "sci":0.314e3, "s":"oops", "true":true, "false":false }'
        ),
        array[
            'i:42,f:42.0,sci:314',
            NULL,
            's:oops',
            'true is not false'
        ],
        'mulitple replacements in many elements. all scalar types');

END; $$ LANGUAGE plpgsql;

-- process_templates(template text, vars jsonb) RETURNS text
CREATE FUNCTION test_replace_placeholders_text() RETURNS SETOF TEXT AS $$ BEGIN
    RETURN NEXT is(process_templates('q{{vars.w}}e', '{"w":"?"}'), 'q?e', 'simple replacement');
END; $$ LANGUAGE plpgsql;

SELECT * FROM runtests();

ROLLBACK;
