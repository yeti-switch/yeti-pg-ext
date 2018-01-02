#include "exported_functions.h"
#include "replace_rand.h"
#include "log.h"

#include <utils/builtins.h>
#include <utils/elog.h>

#define LOG_PREFIX "regexp_replace_rand: "

//regexp_replace(in,rule,result,opt) args indexes
enum {
	ARG_IN = 0,
	ARG_RULE,
	ARG_RESULT,
	ARG_OPT
};

PG_FUNCTION_INFO_V1(regexp_replace_rand_noopt);
Datum regexp_replace_rand_noopt(PG_FUNCTION_ARGS)
{
	Datum ret;
	text *t;
	ErrorData *e;
	bool replaced;

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

	//call regexp_replace() catching exceptions
	PG_TRY();
		ret = textregexreplace_noopt(fcinfo);
		if(VARSIZE(ret)==VARHDRSZ){
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

PG_FUNCTION_INFO_V1(regexp_replace_rand);
Datum regexp_replace_rand(PG_FUNCTION_ARGS)
{
	Datum ret;
	text *t;
	ErrorData *e;
	bool replaced;

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
		if(VARSIZE(ret)==VARHDRSZ){
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
