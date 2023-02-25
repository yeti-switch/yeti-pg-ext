#pragma once

#include <stdlib.h>
#include <netinet/in.h>
#include <stdbool.h>

#define MAX_ENDPOINT_LEN 128
#define MAX_ENDPOINTS 5

typedef struct {
	int id;
	char url[MAX_ENDPOINT_LEN];
	struct sockaddr_in sock_addr;
} endpoint;

struct transport {
	int (*init)(void);
	int (*add_endpoint)(const char* uri);
	int (*get_endpoints_count)(void);
	const endpoint* (*get_endpoint_at_index)(int index);
	int (*set_timeout)(long t);
	int (*shut_down_all_connections)(void);
	int (*remove_all_endpoints)(void);
	int (*send_msg)(const void *buf, size_t len);
	int (*recv_msg)(void *buf, size_t len);
	int (*shutdown)(void);
};

extern const struct transport Transport;
