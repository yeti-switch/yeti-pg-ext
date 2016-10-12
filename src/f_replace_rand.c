#include "exported_functions.h"
#include "replace_rand.h"
#include "log.h"

#define LOG_PREFIX "replace_rand(): "

PG_FUNCTION_INFO_V1(replace_rand);
Datum replace_rand(PG_FUNCTION_ARGS)
{
	bool replaced;
	text *in = PG_GETARG_TEXT_P(0), *out;
	out = replace(in,&replaced);
	if(!out) return get_in_copy(fcinfo);
	if(!replaced) return get_in_copy(fcinfo);
	PG_RETURN_TEXT_P(out);
}
