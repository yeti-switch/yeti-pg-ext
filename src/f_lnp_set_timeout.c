#include "exported_functions.h"
#include "log.h"
#include "shared_vars.h"

#include <nanomsg/nn.h>

PG_FUNCTION_INFO_V1(lnp_set_timeout);
Datum lnp_set_timeout(PG_FUNCTION_ARGS)
{
	int32 nn_request_timeout = PG_GETARG_INT32(0);
	nn_setsockopt(nn_socket_fd,NN_SOL_SOCKET,NN_SNDTIMEO,
				  &nn_request_timeout,sizeof(int));
	nn_setsockopt(nn_socket_fd,NN_SOL_SOCKET,NN_RCVTIMEO,
				  &nn_request_timeout,sizeof(int));
	PG_RETURN_NULL();
}
