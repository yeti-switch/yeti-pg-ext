#pragma once

#define MAX_ENDPOINT_LEN 128
#define MAX_ENDPOINTS 5

extern int nn_socket_fd;

typedef struct {
	int id;
	char url[MAX_ENDPOINT_LEN];
} endpoint;

extern int endpoints_count;
extern endpoint endpoints[MAX_ENDPOINTS];

