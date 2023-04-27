#pragma once

#include <stdlib.h>
#include <stdint.h>
#include "c.h"

#define TAGGED_REQ_VERSION 0
#define CNAM_REQ_VERSION 1

#define MAX_ENDPOINT_LEN 128
#define MAX_ENDPOINTS 5

typedef struct {
	int id;
	char url[MAX_ENDPOINT_LEN];
} endpoint;

typedef struct {
	uint32_t id;
	uint8_t type;
	int32 db_id;
	struct varlena *data;
} request;

typedef struct {
	uint32_t id;
	bool is_confrm; // is short 4 byte response only with 'resp id'
	struct varlena *val;
	char *val_1;
	char *val_2;
} response;

struct resolver {
	int (*init)(void);
	int (*add_endpoint)(const char* uri);
	int (*get_endpoints_count)(void);
	const endpoint* (*get_endpoint_at_index)(int index);
	int (*remove_all_endpoints)(void);
	int (*set_rtt_timeout)(int timeout);
	int (*resolve)(request *req, response *resp, char **error);
};

extern const struct resolver Resolver;
