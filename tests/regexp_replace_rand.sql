set search_path TO 'yeti_ext';

do language plpgsql $$
declare
    -- tests cases: [description, sql, expected_result]
    -- '-' in expected result field means skip result checking
    tests text[] := array[
/*******************************
 * regexp_replace_rand(TEXT,TEXT,TEXT) tests *
 *******************************
 */
        ['valid regexp_replace, no replace_rand',E'regexp_replace_rand(\'a\',\'a\',\'b\')','b'],
        ['valid regexp_replace, with replace_rand',E'regexp_replace_rand(\'a\',\'a\',\'br(4)\')','-'],
        ['regexp_replace exception. no replace_rand',E'regexp_replace_rand(\'a\',\'a[\',\'b\')','a'],
        ['regexp_replace exception. with replace_rand',E'regexp_replace_rand(\'a\',\'a[\',\'br(4)\')','a'],
        ['NULL in',E'regexp_replace_rand(null,\'a\',\'b\')',null],
        ['empty in',E'regexp_replace_rand(\'\',\'a\',\'b\')',''],
        ['NULL rule',E'regexp_replace_rand(\'a\',null,\'b\')','a'],
        ['empty rule',E'regexp_replace_rand(\'a\',\'\',\'b\')','a'],
        ['NULL result',E'regexp_replace_rand(\'a\',\'a\',null)','a'],
        ['empty result',E'regexp_replace_rand(\'a\',\'q\',\'\')','a'],
        ['empty regexp_replace result',E'regexp_replace_rand(\'a\',\'a\',\'\')','a'],
-- table tests
        ['empty rule in table', E'regexp_replace_rand(arg,rule,result) from t', 'a'],
/**************************************************
 * regexp_replace_rand(TEXT,TEXT,TEXT,TEXT) tests *
 **************************************************
 */
        ['valid regexp_replace, with replace_rand',E'regexp_replace_rand(\'a\',\'a\',\'br(4)\',\'g\')','-'],
        ['valid regexp_replace, no replace_rand',E'regexp_replace_rand(\'a\',\'a\',\'b\',\'g\')','b'],
        ['regexp_replace exception. no replace_rand',E'regexp_replace_rand(\'a\',\'a[\',\'b\',\'g\')','a'],
        ['regexp_replace exception. with replace_rand',E'regexp_replace_rand(\'a\',\'a[\',\'br(4)\',\'g\')','a'],
        ['NULL in',E'regexp_replace_rand(null,\'a\',\'b\',\'g\')',null],
        ['empty in',E'regexp_replace_rand(\'\',\'a\',\'b\',\'g\')',''],
        ['NULL rule',E'regexp_replace_rand(\'a\',null,\'b\',\'g\')','a'],
        ['empty rule',E'regexp_replace_rand(\'a\',\'\',\'b\',\'g\')','a'],
        ['NULL result',E'regexp_replace_rand(\'a\',\'a\',null,\'g\')','a'],
        ['empty result',E'regexp_replace_rand(\'a\',\'q\',\'\',\'g\')','a'],
        ['empty regexp_replace result',E'regexp_replace_rand(\'a\',\'a\',\'\',\'g\')','a'],
        ['NULL opt',E'regexp_replace_rand(\'a\',\'a\',\'b\',null)',null],
        ['empty opt',E'regexp_replace_rand(\'a\',\'a\',\'b\',\'\')','b'],
/**************************************************
 * regexp_replace_rand(text_in TEXT[], regexp_rule TEXT, regexp_result TEXT, keep_empty BOOL = FALSE) *
 **************************************************
 */
        ['valid replace, with replace_rand',E'regexp_replace_rand(array[\'a\',\'a\'],\'a\',\'br(4)\')','-'],
        ['valid rreplace, no replace_rand',E'regexp_replace_rand(array[\'a\',\'a\'],\'a\',\'b\')','{b,b}'],
        ['exception. no replace_rand',E'regexp_replace_rand(array[\'a\'],\'a[\',\'b\')', '{a}'],
        ['exception. with replace_rand',E'regexp_replace_rand(array[\'a\'],\'a[\',\'br(4)\')','{a}'],
        ['NULL in',E'regexp_replace_rand(array[null],\'a\',\'b\')','{NULL}'],
        ['empty in',E'regexp_replace_rand(array[\'\'],\'a\',\'b\')','{""}'],
        ['NULL rule',E'regexp_replace_rand(array[\'a\'],null,\'b\')','{a}'],
        ['empty rule',E'regexp_replace_rand(array[\'a\'],\'\',\'b\')','{a}'],
        ['NULL result',E'regexp_replace_rand(array[\'a\'],\'a\',null)','{a}'],
        ['empty result',E'regexp_replace_rand(array[\'a\'],\'q\',\'\')','{a}'],
        ['empty regexp_replace result',E'regexp_replace_rand(array[\'a\'],\'a\',\'\')','{a}'],
        ['multi-in, single-token',E'regexp_replace_rand(array[\'abba\',\'yabba\'],\'b+\',\'*\')','{a*a,ya*a}'],
        ['multi-in, multi-token',E'regexp_replace_rand(array[\'qwe\',\'rty\',\'asd\'],\'(q)||(r)\',\'\\1*||\\1^\')','{q*we,r^ty,asd}']
    ];
    v text[];
    ret text;
begin
    create temporary table t(arg text, rule text, result text) on commit drop;
    insert into t VALUES('a','','b');

    <<"test loop">>
    foreach v slice 1 in array tests loop
        execute 'select ' || v[2] into ret;
        raise notice '% -> "%" (expected "%") //%', v[2],ret,v[3],v[1];
        if ((v[3] is null) or v[3]!='-') and (((v[3] is null) != (ret is null)) or v[3]!=ret) then
            raise exception 'got "%" while expected "%" for the last test case', ret, v[3];
        end if;
    end loop "test loop";
end $$;
