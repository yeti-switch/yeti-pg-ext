#include "exported_functions.h"
#include "log.h"
#include "resolver.h"
#include "funcapi.h"
#include <string.h>

#define LOG_PREFIX "lnp_endpoints_show: "

PG_FUNCTION_INFO_V1(lnp_endpoints_show);
Datum lnp_endpoints_show(PG_FUNCTION_ARGS)
{
	int i;
	FuncCallContext *ctx;
	AttInMetadata *attinmeta;
	TupleDesc tdesc;

	if (SRF_IS_FIRSTCALL()){
		MemoryContext mctx;

		ctx = SRF_FIRSTCALL_INIT();

		mctx = MemoryContextSwitchTo(ctx->multi_call_memory_ctx);
		ctx->max_calls = Resolver.get_endpoints_count();

		if (get_call_result_type(fcinfo, NULL, &tdesc) != TYPEFUNC_COMPOSITE)
			ereport(ERROR,
					(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
					errmsg("function returning record called in context "
						   "that cannot accept type record")));

		attinmeta = TupleDescGetAttInMetadata(tdesc);
		ctx->attinmeta = attinmeta;

		MemoryContextSwitchTo(mctx);
	}

	ctx = SRF_PERCALL_SETUP();

	i = ctx->call_cntr;
	attinmeta = ctx->attinmeta;

	if(i < ctx->max_calls){
		char **v;
		HeapTuple t;
		Datum r;

		v = (char **) palloc(2 * sizeof(char *));
		v[0] = (char *) palloc(16*sizeof(char));
		v[1] = (char *) palloc(MAX_ENDPOINT_LEN*sizeof(char));

		const endpoint *ep = Resolver.get_endpoint_at_index(i);

		if (ep == NULL) {
			PG_RETURN_NULL();
		}

		snprintf(v[0], 16, "%d", ep->id);
		strncpy(v[1], ep->url, MAX_ENDPOINT_LEN);

		t = BuildTupleFromCStrings(attinmeta, v);
		r = HeapTupleGetDatum(t);

		pfree(v[0]);
		pfree(v[1]);
		pfree(v);

		SRF_RETURN_NEXT(ctx, r);
	}

	SRF_RETURN_DONE(ctx);
}
