#include "exported_functions.h"
#include "log.h"
#include "shared_vars.h"
#include "f_tbf_rate_check.h"

#include <strings.h>
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>
#include <errno.h>

#define LOG_PREFIX ""

#define nn_error(fmt, ...) err(fmt, ## __VA_ARGS__)

void _PG_init(void)
{
	//init shared variables
	endpoints_count = 0;
	bzero(endpoints,sizeof(endpoint)*MAX_ENDPOINTS);

	//create socket
	if((nn_socket_fd = nn_socket(AF_SP, NN_REQ))<0){
		nn_error("cant create nn socket");
	}

	tbf_init();
}
