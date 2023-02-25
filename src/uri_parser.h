#pragma once

#define SOCKADDR_SIZE 128
#define PROTOCOL_SIZE 16
#define HOST_SIZE 64

typedef struct {
	char proto[PROTOCOL_SIZE + 1];
	char host[HOST_SIZE + 1];
	int port;
} UriComponents;

int parseAddr(const char *addr, UriComponents *out);
