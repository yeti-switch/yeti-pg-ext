#pragma once

#include <stdlib.h>
#include <netinet/in.h>
#include <stdbool.h>

struct transport {
	int (*init)(void);
	int (*get_socket_fd)();
	int (*set_timeout)(long t);
	int (*send_data)(const void *buf, size_t len, const char *host, int port);
	int (*recv_data)(void *buf, size_t len);
	int (*shutdown)(void);
};

extern const struct transport Transport;
