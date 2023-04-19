#include "exported_functions.h"
#include "log.h"
#include "transport.h"

#include <utils/array.h>

#define LOG_PREFIX "lnp_endpoints_set: "

PG_FUNCTION_INFO_V1(lnp_endpoints_set);
Datum lnp_endpoints_set(PG_FUNCTION_ARGS)
{
	Datum uri;
	char c_uri[MAX_ENDPOINT_LEN];
	bool is_null;
	ArrayIterator it;
	ArrayType *input= PG_GETARG_ARRAYTYPE_P(0);

	Transport.remove_all_endpoints();

	//apply new ones

#if PGVER > 904
	it = array_create_iterator(input,0,NULL);
#else
	it = array_create_iterator(input,0);
#endif

	while(array_iterate(it,&uri,&is_null)){
		if(is_null) continue;
		memset(&c_uri, 0, MAX_ENDPOINT_LEN);
		memcpy(&c_uri, VARDATA_ANY(uri), VARSIZE_ANY_EXHDR(uri));
		Transport.add_endpoint(c_uri);
	}

	PG_RETURN_NULL();
}
