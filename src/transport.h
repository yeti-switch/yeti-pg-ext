#pragma once

#include <stdlib.h>
#include <netinet/in.h>
#include <stdbool.h>

struct transport {
	int (*init)(void);
	int (*init_socket)(void);
	int (*get_socket_fd)();
	int (*set_timeout)(long t);
	int (*send_data)(const void *buf, size_t len, const struct sockaddr_in *addr, char **error);
	int (*recv_data)(void *buf, size_t len, char **error);
	int (*shutdown_socket)(void);
};

extern const struct transport Transport;
