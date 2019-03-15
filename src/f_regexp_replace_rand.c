#include "exported_functions.h"
#include "replace_rand.h"
#include "log.h"

#include <utils/builtins.h>
#include <utils/elog.h>
#include <regex/regex.h>

#define LOG_PREFIX "regexp_replace_rand: "

#define REGEXP_SPLIT_TOKEN_1 '|'
#define REGEXP_SPLIT_TOKEN_2 '|'
#define REGEXP_SPLIT_TOKEN_LEN 2
#define REGEXP_SPLIT_MAX_TOKENS 10

//regexp_replace(in,rule,result,opt) args indexes
#define ARG_IN 0
#define ARG_RULE 1
#define ARG_RESULT 2
#define ARG_OPT 3

#define OPT_ARG_KEEP_EMPTY 4
#define NOOPT_ARG_KEEP_EMPTY 3

static inline const char *search_split_token(const char *s, size_t len)
{
	const char *p = memchr(s,REGEXP_SPLIT_TOKEN_1,len);
	if(!p)
		return NULL;
	if(p+REGEXP_SPLIT_TOKEN_LEN > s+len) {
		return NULL;
	}
	if(*(p+1)==REGEXP_SPLIT_TOKEN_2) {
		return p;
	}
	return NULL;
}

static inline void replace_arg(PG_FUNCTION_ARGS, int arg_idx, text *t, const char *start, const char *end)
{
	int len;

	len = end-start;
	if(t) pfree(t);
	t = (text *)palloc(len + VARHDRSZ);
	if(!t) {
		err("no memory for chunk");
		return;
	}

	SET_VARSIZE(t, len + VARHDRSZ);
	memcpy(VARDATA(t),start,len);
	PG_GETARG_DATUM(arg_idx) = PointerGetDatum(t);
}


static Datum apply_textregexreplace_noopt(PG_FUNCTION_ARGS, bool *matched, bool final, bool keep_empty)
{
	Datum ret;
	ErrorData *e;

	text *pattern, *data;
	regex_t		match_regexp;
	pg_wchar	*wide_pattern, *wide_data;
	int			wide_pattern_len, wide_data_len;
	int			orig_len;
	int			reg_result;

	ret = 0;

	*matched = false;

	wide_pattern = NULL;
	pattern = PG_GETARG_TEXT_P(ARG_RULE);
	orig_len = VARSIZE_ANY_EXHDR(pattern);
	wide_pattern = (pg_wchar *) palloc(sizeof(pg_wchar) * (orig_len + 1));
	wide_pattern_len = pg_mb2wchar_with_len(VARDATA_ANY(pattern), wide_pattern, orig_len);

	reg_result = pg_regcomp(&match_regexp,wide_pattern,wide_pattern_len,
							REG_ADVANCED,PG_GET_COLLATION());
	if(reg_result != REG_OKAY) {
		pfree(wide_pattern);
		return ret;
	}

	pfree(wide_pattern);

	data = PG_GETARG_TEXT_P(ARG_IN);
	orig_len = VARSIZE_ANY_EXHDR(data);
	wide_data = (pg_wchar *) palloc(sizeof(pg_wchar) * (orig_len + 1));
	wide_data_len = pg_mb2wchar_with_len(VARDATA_ANY(data), wide_data, orig_len);

	reg_result = pg_regexec(&match_regexp, wide_data, wide_data_len, 0, NULL, 0, NULL, 0);

	pfree(wide_data);
	pg_regfree(&match_regexp);

	if (reg_result != REG_OKAY) {
		if(final) {
			ret = get_in_copy(fcinfo);
		}
		return ret;
	}

	*matched = true;

	PG_TRY();

		ret = textregexreplace_noopt(fcinfo);
		if(!keep_empty && VARSIZE(ret)==VARHDRSZ) {
			dbg("empty regex_replace() result. return input if final");
			pfree(DatumGetPointer(ret));
			if(final) {
				ret = get_in_copy(fcinfo);
			} else {
				ret = 0;
			}
		}

	PG_CATCH();

		/* TODO: find the way to get reference
		 * to the top exception data without copying */
		e = CopyErrorData();
		dbg("exception in regexp_replace() %s. return input if final",e->message);
		FreeErrorData(e);

		EmitErrorReport();
		FlushErrorState();

		if(final) {
			ret = get_in_copy(fcinfo);
		} else {
			ret = 0;
		}

	PG_END_TRY();

	return ret;
}

PG_FUNCTION_INFO_V1(regexp_replace_rand_noopt);
Datum regexp_replace_rand_noopt(PG_FUNCTION_ARGS)
{
	Datum ret;
	text *t;
	bool replaced;
	bool matched;
	bool keep_empty = false;

	const char *rule_ptr, *rule_token_pos, *rule_end,
			   *result_ptr, *result_token_pos, *result_end;
	text *rule_chunk, *result_chunk;
	int n;

	if(PG_ARGISNULL(ARG_IN)) {
		dbg("input is null. return null");
		PG_RETURN_NULL();
	}

	if(PG_ARGISNULL(ARG_RULE)){
		dbg("rule is null. return input");
		return get_in_copy(fcinfo);
	}

	if(PG_ARGISNULL(ARG_RESULT)){
		dbg("result is null. return input");
		return get_in_copy(fcinfo);
	}

	if(PG_NARGS() > NOOPT_ARG_KEEP_EMPTY && !PG_ARGISNULL(NOOPT_ARG_KEEP_EMPTY))
		keep_empty = PG_GETARG_BOOL(NOOPT_ARG_KEEP_EMPTY);

	/*if(VARSIZE(PG_GETARG_DATUM(ARG_IN))==VARHDRSZ){
		dbg("input is empty. return input");
		return get_in_copy(fcinfo);
	}*/

	if(VARSIZE(PG_GETARG_DATUM(ARG_RULE))==VARHDRSZ){
		dbg("rule is empty. return input");
		return get_in_copy(fcinfo);
	}

	t = replace(PG_GETARG_TEXT_P(ARG_RESULT),&replaced);
	if(!t) {
		dbg("replace failed. return input");
		return get_in_copy(fcinfo);
	}

	if(replaced) {
		/* modify fcinfo to call regexp_replace()
		* replace pointer to the result field */
		PG_GETARG_DATUM(ARG_RESULT) = PointerGetDatum(t);
	}

	rule_ptr = (const char *)VARDATA(PG_GETARG_TEXT_P(ARG_RULE));
	rule_end = rule_ptr + VARSIZE(PG_GETARG_TEXT_P(ARG_RULE)) - VARHDRSZ;
	rule_token_pos = NULL;
	rule_chunk = 0;

	result_ptr = (const char *)VARDATA(PG_GETARG_TEXT_P(ARG_RESULT));
	result_end = result_ptr + VARSIZE(PG_GETARG_TEXT_P(ARG_RESULT)) - VARHDRSZ;
	result_token_pos = NULL;
	result_chunk = 0;

	matched = false;

	n = 0;
	ret = 0;

	//iterate over rule/result chunks
	while((rule_token_pos = search_split_token(rule_ptr, rule_end-rule_ptr))!=NULL) {
		replace_arg(fcinfo, ARG_RULE, rule_chunk, rule_ptr, rule_token_pos);

		result_token_pos = search_split_token(result_ptr, result_end-result_ptr);
		if(!result_token_pos) {
			if(n > 0) {
				replace_arg(fcinfo, ARG_RESULT, result_chunk, result_ptr, result_end);
			}
			ret = apply_textregexreplace_noopt(fcinfo,&matched,true,keep_empty);
			goto out;
		}

		replace_arg(fcinfo, ARG_RESULT, result_chunk, result_ptr, result_token_pos);

		ret = apply_textregexreplace_noopt(fcinfo,&matched,false,keep_empty);
		if(matched) {
			goto out;
		}

		n++;

		if(n >= REGEXP_SPLIT_MAX_TOKENS-1) {
			goto out;
		}

		rule_ptr = rule_token_pos+REGEXP_SPLIT_TOKEN_LEN;
		if(rule_ptr >= rule_end) {
			goto out;
		}

		result_ptr = result_token_pos+REGEXP_SPLIT_TOKEN_LEN;
		if(result_ptr >= result_end) {
			goto out;
		}
	}

	//process no tokens/tail cases
	result_token_pos = search_split_token(result_ptr, result_end-result_ptr);
	if(result_token_pos) {
		replace_arg(fcinfo, ARG_RESULT, result_chunk, result_ptr, result_token_pos);
	}
	if(n) {
		replace_arg(fcinfo, ARG_RULE, rule_chunk, rule_ptr, rule_end);
		if(!result_token_pos) {
			replace_arg(fcinfo, ARG_RESULT, result_chunk, result_ptr, result_end);
		}
	}
	ret = apply_textregexreplace_noopt(fcinfo,&matched,true,keep_empty);

out:
	if(replaced) pfree(t);
	if(rule_chunk) pfree(rule_chunk);
	if(result_chunk) pfree(result_chunk);

	if(!ret) ret = get_in_copy(fcinfo);

	return ret;
}

PG_FUNCTION_INFO_V1(regexp_replace_rand);
Datum regexp_replace_rand(PG_FUNCTION_ARGS)
{
	Datum ret;
	text *t;
	ErrorData *e;
	bool replaced;
	bool keep_empty = false;

	if(PG_ARGISNULL(ARG_IN)) {
		dbg("input is null. return null");
		PG_RETURN_NULL();
	}

	if(PG_ARGISNULL(ARG_RULE)){
		dbg("rule is null. return input");
		return get_in_copy(fcinfo);
	}

	if(PG_ARGISNULL(ARG_OPT)) {
		dbg("options is null. return null");
		PG_RETURN_NULL();
	}

	if(PG_ARGISNULL(ARG_RESULT)){
		dbg("result is null. return input");
		return get_in_copy(fcinfo);
	}

	/*if(VARSIZE(PG_GETARG_DATUM(ARG_IN))==VARHDRSZ){
		dbg("input is empty. return input");
		return get_in_copy(fcinfo);
	}*/

	if(VARSIZE(PG_GETARG_DATUM(ARG_RULE))==VARHDRSZ){
		dbg("rule is empty. return input");
		return get_in_copy(fcinfo);
	}

	if(PG_NARGS() > OPT_ARG_KEEP_EMPTY && !PG_ARGISNULL(OPT_ARG_KEEP_EMPTY))
		keep_empty = PG_GETARG_BOOL(OPT_ARG_KEEP_EMPTY);

	t = replace(PG_GETARG_TEXT_P(ARG_RESULT),&replaced);
	if(!t) {
		dbg("replace failed. return input");
		return get_in_copy(fcinfo);
	}

	if(replaced) {
		/* modify fcinfo to call regexp_replace()
		* replace pointer to the result field */
		PG_GETARG_DATUM(ARG_RESULT) = PointerGetDatum(t);
	}

	//call regexp_replace() catching exceptions
	PG_TRY();
		ret = textregexreplace(fcinfo);
		if(!keep_empty && VARSIZE(ret)==VARHDRSZ) {
			dbg("empty regex_replace() result. return input");
			pfree(DatumGetPointer(ret));
			ret = get_in_copy(fcinfo);
		}
	PG_CATCH();
		/* TODO: find the way to get reference
		 * to the top exception data without copying */
		e = CopyErrorData();
		dbg("exception in regexp_replace() %s. return input",e->message);
		FreeErrorData(e);

		EmitErrorReport();
		FlushErrorState();

		ret = get_in_copy(fcinfo);
	PG_END_TRY();

	if(replaced) pfree(t);

	return ret;
}
