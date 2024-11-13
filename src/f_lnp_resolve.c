#include "exported_functions.h"
#include "log.h"
#include "resolver.h"
#include "endpoints_cache.h"

#include "funcapi.h"
#include "utils/builtins.h"

#include <stdlib.h>

#define LOG_PREFIX "lnp_resolve: "

PG_FUNCTION_INFO_V1(lnp_resolve_cnam);
Datum lnp_resolve_cnam(PG_FUNCTION_ARGS) {
	request req;
	response resp;
	char *error = NULL;
	char *resp_moc;
	bool error_moc;

	memset(&req, 0, sizeof(request));
	req.db_id = PG_GETARG_INT32(0);
	req.type = CNAM_REQ_VERSION;
	req.data = PG_GETARG_TEXT_P(1);

	// find mocking response if needed
	if (EndpointsCache.find(req.data, &resp_moc, &error_moc) == 0) {
		if (error_moc) {
			warn("%s", resp_moc);
			PG_RETURN_NULL();
		}
		return CStringGetTextDatum(resp_moc);
	}

	memset(&resp, 0, sizeof(response));
	if (Resolver.resolve(&req, &resp, &error) != 0) {
		warn("%s", error ? error : "generic failure");
		PG_RETURN_NULL();
	}

	PG_RETURN_TEXT_P(resp.val);
}

PG_FUNCTION_INFO_V1(lnp_resolve_tagged);
Datum lnp_resolve_tagged(PG_FUNCTION_ARGS) {
	request req;
	response resp;
	char *error = NULL;
	TupleDesc tdesc;
	HeapTuple t;
	AttInMetadata *attinmeta;
	char *v[2], *resp_moc;
	bool error_moc;

	if (get_call_result_type(fcinfo, NULL, &tdesc) != TYPEFUNC_COMPOSITE) {
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context "
					   "that cannot accept type record")));
	}

	memset(&req, 0, sizeof(request));
	req.db_id = PG_GETARG_INT32(0);
	req.type = TAGGED_REQ_VERSION;
	req.data = PG_GETARG_TEXT_P(1);

	// find mocking response if needed
	if (EndpointsCache.find(req.data, &resp_moc, &error_moc) == 0) {
		if (error_moc) {
			warn("%s", resp_moc);
			PG_RETURN_NULL();
		}

		v[0] = resp_moc; //lrn
		v[1] = NULL; // tag
		attinmeta = TupleDescGetAttInMetadata(tdesc);
		t = BuildTupleFromCStrings(attinmeta, v);
		return HeapTupleGetDatum(t);
	}

	memset(&resp, 0, sizeof(response));
	if (Resolver.resolve(&req, &resp, &error) != 0) {
		warn("%s", error ? error : "generic failure");
		PG_RETURN_NULL();
	}

	v[0] = resp.val_1; // lrn
	v[1] = resp.val_2; // tag

	attinmeta = TupleDescGetAttInMetadata(tdesc);
	t = BuildTupleFromCStrings(attinmeta, v);

	if (resp.val_1) pfree(resp.val_1);
	if (resp.val_2) pfree(resp.val_2);

	return HeapTupleGetDatum(t);
}

PG_FUNCTION_INFO_V1(lnp_resolve_tagged_with_error);
Datum lnp_resolve_tagged_with_error(PG_FUNCTION_ARGS) {
	request req;
	response resp;
	char *error = NULL;
	TupleDesc tdesc;
	HeapTuple t;
	AttInMetadata *attinmeta;
	char *v[3], *resp_moc;
	bool error_moc;

	if (get_call_result_type(fcinfo, NULL, &tdesc) != TYPEFUNC_COMPOSITE) {
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context "
					   "that cannot accept type record")));
	}

	memset(&req, 0, sizeof(request));
	req.db_id = PG_GETARG_INT32(0);
	req.type = TAGGED_REQ_VERSION;
	req.data = PG_GETARG_TEXT_P(1);

	// find mocking response if needed
	if (EndpointsCache.find(req.data, &resp_moc, &error_moc) == 0) {
		if (error_moc) {
			v[0] = NULL; // lrn
			v[1] = NULL; // tag
			v[2] = resp_moc; // error
		} else {
			v[0] = resp_moc; //lrn
			v[1] = NULL; // tag
			v[2] = NULL; // error
		}

		attinmeta = TupleDescGetAttInMetadata(tdesc);
		t = BuildTupleFromCStrings(attinmeta, v);
		return HeapTupleGetDatum(t);
	}

	memset(&resp, 0, sizeof(response));
	if (Resolver.resolve(&req, &resp, &error) != 0) {
		v[0] = NULL; // lrn
		v[1] = NULL; // tag
		v[2] = error; // error
	} else {
		v[0] = resp.val_1; // lrn
		v[1] = resp.val_2; // tag
		v[2] = NULL; // error
	}

	attinmeta = TupleDescGetAttInMetadata(tdesc);
	t = BuildTupleFromCStrings(attinmeta, v);

	if(v[2]) v[2] = NULL;
	if(v[1]) pfree(v[1]);
	if(v[0]) pfree(v[0]);

	return HeapTupleGetDatum(t);
}
