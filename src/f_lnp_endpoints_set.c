#include "exported_functions.h"
#include "log.h"
#include "shared_vars.h"

#include <nanomsg/nn.h>
#include <utils/array.h>

#define LOG_PREFIX "lnp_endpoints_set: "

#define nn_warn(fmt, ...) warn(fmt": %s", ## __VA_ARGS__, nn_strerror(errno))

PG_FUNCTION_INFO_V1(lnp_endpoints_set);
Datum lnp_endpoints_set(PG_FUNCTION_ARGS)
{
	int id,i;
	Datum uri;
	char *e_uri;
	bool is_null;
	ArrayIterator it;
	ArrayType *input= PG_GETARG_ARRAYTYPE_P(0);

	//shutdown old endpoints
	for(i=0;i<endpoints_count;i++)
		nn_shutdown(nn_socket_fd,endpoints[i].id);
	bzero(endpoints,sizeof(endpoint)*endpoints_count);
	endpoints_count = 0;

	//apply new ones

#if PGVER > 904
	it = array_create_iterator(input,0,NULL);
#else
	it = array_create_iterator(input,0);
#endif

	while(array_iterate(it,&uri,&is_null)){
		if(is_null) continue;

		if(endpoints_count==MAX_ENDPOINTS){
			warn("endpoints count limit (%d) reached",MAX_ENDPOINTS);
			continue;
		}

		e_uri = endpoints[endpoints_count].url;
		memcpy(e_uri,VARDATA(uri),VARSIZE(uri)-VARHDRSZ);

		id = nn_connect(nn_socket_fd, e_uri);
		if(id < 0){
			nn_warn("can't add endpoint '%s'",e_uri);
			bzero(e_uri,VARSIZE(uri)-VARHDRSZ);
			continue;
		}

		endpoints[endpoints_count].id = id;
		endpoints_count++;
	}
	PG_RETURN_NULL();
}
