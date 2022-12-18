#include "exported_functions.h"
#include "log.h"
#include "shared_vars.h"
#include "f_tbf_rate_check.h"

#include <nanomsg/nn.h>

#define LOG_PREFIX ""

void _PG_fini(void){
	nn_shutdown(nn_socket_fd,0);
	nn_close(nn_socket_fd);

	tbf_fini();
}
