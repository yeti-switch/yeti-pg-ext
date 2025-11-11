BEGIN;

-- CREATE EXTENSION IF NOT EXISTS yeti WITH SCHEMA yeti_ext;
set search_path TO public,yeti_ext;

-- Load the TAP functions.
-- TODO: find the way to include pgtap.sql for the related major version (:SERVER_VERSION_NUM/10000)
\i /usr/share/postgresql/16/extension/pgtap.sql

-- OR
CREATE FUNCTION test_tags_compare_or() RETURNS SETOF TEXT AS $$ BEGIN
    -- a: notag
    RETURN NEXT is(tag_compare(array[]::integer[], array[]::integer[]), 3, 'notags_notags');
    RETURN NEXT is(tag_compare(array[]::integer[], array[1]),           0, 'notags_1');
    RETURN NEXT is(tag_compare(array[]::integer[], array[1,2]),         0, 'notags_12');
    RETURN NEXT is(tag_compare(array[]::integer[], array[2]),           0, 'notags_2');

    -- a: 1
    RETURN NEXT is(tag_compare(array[1], array[]::integer[]), 0, '1_notags');
    RETURN NEXT is(tag_compare(array[1], array[1]),           3, '1_1');
    RETURN NEXT is(tag_compare(array[1], array[1,2]),         0, '1_12');
    RETURN NEXT is(tag_compare(array[1], array[2]),           0, '1_2');

    -- a: 1,2
    RETURN NEXT is(tag_compare(array[1,2], array[]::integer[]), 0, '12_notags');
    RETURN NEXT is(tag_compare(array[1,2], array[1]),           2, '12_1');
    RETURN NEXT is(tag_compare(array[1,2], array[1,2]),         3, '12_12');
    RETURN NEXT is(tag_compare(array[1,2], array[2]),           2, '12_2');

    -- a: 1,2,any
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[]::integer[]), 0, '12any_notag');
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[1]),           2, '12any_1');
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[1,2]),         3, '12any_12');
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[2]),           2, '12any_2');

    -- a: 1,3
    RETURN NEXT is(tag_compare(array[1,3], array[]::integer[]), 0, '13_notags');
    RETURN NEXT is(tag_compare(array[1,3], array[1]),           2, '13_1');
    RETURN NEXT is(tag_compare(array[1,3], array[1,2]),         0, '13_12');
    RETURN NEXT is(tag_compare(array[1,3], array[2]),           0, '13_2');

    -- a: 1,any
    RETURN NEXT is(tag_compare(array[1,NULL], array[]::integer[]), 0, '1any_notags');
    RETURN NEXT is(tag_compare(array[1,NULL], array[1]),           3, '1any_1');
    RETURN NEXT is(tag_compare(array[1,NULL], array[1,2]),         1, '1any_12');
    RETURN NEXT is(tag_compare(array[1,NULL], array[2]),           1, '1any_2');

    -- a: 2
    RETURN NEXT is(tag_compare(array[2], array[]::integer[]), 0, '2_notags');
    RETURN NEXT is(tag_compare(array[2], array[1]),           0, '2_1');
    RETURN NEXT is(tag_compare(array[2], array[1,2]),         0, '2_12');
    RETURN NEXT is(tag_compare(array[2], array[2]),           3, '2_2');

    -- a: 2,any
    RETURN NEXT is(tag_compare(array[2,NULL], array[]::integer[]), 0, '2any_notags');
    RETURN NEXT is(tag_compare(array[2,NULL], array[1]),           1, '2any_1');
    RETURN NEXT is(tag_compare(array[2,NULL], array[1,2]),         1, '2any_12');
    RETURN NEXT is(tag_compare(array[2,NULL], array[2]),           3, '2any_2');

    -- any
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[]::integer[]), 0, 'any_notags');
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[1]),           1, 'any_1');
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[1,2]),         1, 'any_12');
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[2]),           1, 'any_2');
END; $$ LANGUAGE plpgsql;

-- AND
CREATE FUNCTION test_tags_compare_and() RETURNS SETOF TEXT AS $$ BEGIN
    -- a: notag
    RETURN NEXT is(tag_compare(array[]::integer[], array[]::integer[], 1::smallint), 3, 'notags_notags');
    RETURN NEXT is(tag_compare(array[]::integer[], array[1], 1::smallint),           0, 'notags_1');
    RETURN NEXT is(tag_compare(array[]::integer[], array[1,2], 1::smallint),         0, 'notags_12');
    RETURN NEXT is(tag_compare(array[]::integer[], array[2], 1::smallint),           0, 'notags_2');

    -- a: 1
    RETURN NEXT is(tag_compare(array[1], array[]::integer[], 1::smallint), 0, '1_notags');
    RETURN NEXT is(tag_compare(array[1], array[1], 1::smallint),           3, '1_1');
    RETURN NEXT is(tag_compare(array[1], array[1,2], 1::smallint),         0, '1_12');
    RETURN NEXT is(tag_compare(array[1], array[2], 1::smallint),           0, '1_2');

    -- a: 1,2
    RETURN NEXT is(tag_compare(array[1,2], array[]::integer[], 1::smallint), 0, '12_notags');
    RETURN NEXT is(tag_compare(array[1,2], array[1], 1::smallint),           0, '12_1');
    RETURN NEXT is(tag_compare(array[1,2], array[1,2], 1::smallint),         3, '12_12');
    RETURN NEXT is(tag_compare(array[1,2], array[2], 1::smallint),           0, '12_2');

    -- a: 1,2,any
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[]::integer[], 1::smallint), 0, '12any_notag');
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[1], 1::smallint),           0, '12any_1');
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[1,2], 1::smallint),         3, '12any_12');
    RETURN NEXT is(tag_compare(array[1,2,NULL], array[2], 1::smallint),           0, '12any_2');

    -- a: 1,3
    RETURN NEXT is(tag_compare(array[1,3], array[]::integer[], 1::smallint), 0, '13_notags');
    RETURN NEXT is(tag_compare(array[1,3], array[1], 1::smallint),           0, '13_1');
    RETURN NEXT is(tag_compare(array[1,3], array[1,2], 1::smallint),         0, '13_12');
    RETURN NEXT is(tag_compare(array[1,3], array[2], 1::smallint),           0, '13_2');

    -- a: 1,any
    RETURN NEXT is(tag_compare(array[1,NULL], array[]::integer[], 1::smallint), 0, '1any_notags');
    RETURN NEXT is(tag_compare(array[1,NULL], array[1], 1::smallint),           3, '1any_1');
    RETURN NEXT is(tag_compare(array[1,NULL], array[1,2], 1::smallint),         1, '1any_12');
    RETURN NEXT is(tag_compare(array[1,NULL], array[2], 1::smallint),           0, '1any_2');

    -- a: 2
    RETURN NEXT is(tag_compare(array[2], array[]::integer[], 1::smallint), 0, '2_notags');
    RETURN NEXT is(tag_compare(array[2], array[1], 1::smallint),           0, '2_1');
    RETURN NEXT is(tag_compare(array[2], array[1,2], 1::smallint),         0, '2_12');
    RETURN NEXT is(tag_compare(array[2], array[2], 1::smallint),           3, '2_2');

    -- a: 2,any
    RETURN NEXT is(tag_compare(array[2,NULL], array[]::integer[], 1::smallint), 0, '2any_notags');
    RETURN NEXT is(tag_compare(array[2,NULL], array[1], 1::smallint),           0, '2any_1');
    RETURN NEXT is(tag_compare(array[2,NULL], array[1,2], 1::smallint),         1, '2any_12');
    RETURN NEXT is(tag_compare(array[2,NULL], array[2], 1::smallint),           3, '2any_2');

    -- any
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[]::integer[], 1::smallint), 0, 'any_notags');
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[1], 1::smallint),           1, 'any_1');
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[1,2], 1::smallint),         1, 'any_12');
    RETURN NEXT is(tag_compare(array[NULL]::integer[], array[2], 1::smallint),           1, 'any_2');
END; $$ LANGUAGE plpgsql;

SELECT * FROM runtests();

ROLLBACK;
