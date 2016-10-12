/*******************************
 * regexp_replace_rand(TEXT,TEXT,TEXT) tests *
 *******************************

 */

-- valid regexp_replace, with replace_rand
select regexp_replace_rand('a','a','br(4)');
-- valid regexp_replace, no replace_rand
select regexp_replace_rand('a','a','b');

-- regexp_replace exception. no replace_rand
select regexp_replace_rand('a','a[','b');
-- regexp_replace exception. with replace_rand
select regexp_replace_rand('a','a[','br(4)');

-- NULL in
select regexp_replace_rand(NULL,'a','b');
-- empty in
select regexp_replace_rand('','a','b');

-- NULL rule
select regexp_replace_rand('a',NULL,'b');
-- empty rule
select regexp_replace_rand('a','','b');

-- NULL result
select regexp_replace_rand('a','a',NULL);
-- empty result
select regexp_replace_rand('a','q','');
-- empty regexp_replace result
select regexp_replace_rand('a','a','');

/**************************************************
 * regexp_replace_rand(TEXT,TEXT,TEXT,TEXT) tests *
 **************************************************

 */

-- valid regexp_replace, with replace_rand
select regexp_replace_rand('a','a','br(4)','g');
-- valid regexp_replace, no replace_rand
select regexp_replace_rand('a','a','b','g');

-- regexp_replace exception. no replace_rand
select regexp_replace_rand('a','a[','b','g');
-- regexp_replace exception. with replace_rand
select regexp_replace_rand('a','a[','br(4)','g');

-- NULL in
select regexp_replace_rand(NULL,'a','b','g');
-- empty in
select regexp_replace_rand('','a','b','g');

-- NULL rule
select regexp_replace_rand('a',NULL,'b','g');
-- empty rule
select regexp_replace_rand('a','','b','g');

-- NULL result
select regexp_replace_rand('a','a',NULL,'g');
-- empty result
select regexp_replace_rand('a','q','','g');
-- empty regexp_replace result
select regexp_replace_rand('a','a','','g');

-- NULL opt
select regexp_replace_rand('a','a','b',NULL);

-- empty opt
select regexp_replace_rand('a','a','b','');

