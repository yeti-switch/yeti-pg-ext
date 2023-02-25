#include "uri_parser.h"

#include <string.h>
#include <stdlib.h>

int parseAddr(const char *addr, UriComponents *out) {
	// check whether address is valid
	if (addr == NULL || strlen(addr) == 0 || strlen(addr) >= SOCKADDR_SIZE)
		return -1;

	// check uri components
	if (out == NULL)
		return -1;

	// fill uri components
	memset(out, 0, sizeof(UriComponents));

	const char *delim;
	size_t delim_sz;
	size_t comp_sz;

	// parse protocol
	delim = strstr(addr, "://");
	delim_sz = strlen("://");

	if (delim != NULL)
	{
		comp_sz = delim - addr;

		if (comp_sz > 0 && comp_sz <= PROTOCOL_SIZE)
			strncpy(out->proto, addr, comp_sz);

		addr += comp_sz + delim_sz;
	}

	// parse host
	delim = strchr(addr, ':');
	delim_sz = sizeof(char);

	if (delim != NULL)
	{
		comp_sz = delim - addr;

		if (comp_sz > 0 && comp_sz <= HOST_SIZE)
			strncpy(out->host, addr, comp_sz);

		addr += comp_sz + delim_sz;

		// parse port
		out->port = atoi(addr);
	}
	else
	{
		comp_sz = strlen(addr);

		if (comp_sz > 0 && comp_sz <= HOST_SIZE)
			strncpy(out->host, addr, comp_sz);

		strncpy(out->host, addr, strlen(addr));
	}

	return 0;
}
