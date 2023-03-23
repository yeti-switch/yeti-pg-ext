#include "transport.h"
#include "log.h"
#include "uri_parser.h"

#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <strings.h>
#include <arpa/inet.h>
#include <utils/array.h>
#include <errno.h>
#include <stdbool.h>

#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

#define LOG_PREFIX "transport: "

/*
 * local vars
*/

int nn_socket_fd;
int udp_socket_fd;
bool udp_mode;
int endpoints_count;
endpoint endpoints[MAX_ENDPOINTS];
int curr_endpoint_index;

/*
 * func prototypes
*/

int __tr_init(void);
int __tr_add_endpoint(const char* uri);
int __tr_get_endpoints_count(void);
const endpoint *__tr_get_endpoint_at_index(int index);
int __tr_set_timeout(long t);
int __tr_shut_down_all_connections(void);
int __tr_remove_all_endpoints(void);
int __tr_send_msg(const void *buf, size_t len);
int __tr_recv_msg(void *buf, size_t len);
int __tr_shutdown(void);

/*
 * 'transport' struct
*/

const struct transport Transport = {
	.init = __tr_init,
	.add_endpoint = __tr_add_endpoint,
	.get_endpoints_count = __tr_get_endpoints_count,
	.get_endpoint_at_index = __tr_get_endpoint_at_index,
	.set_timeout = __tr_set_timeout,
	.shut_down_all_connections = __tr_shut_down_all_connections,
	.remove_all_endpoints = __tr_remove_all_endpoints,
	.send_msg = __tr_send_msg,
	.recv_msg = __tr_recv_msg,
	.shutdown = __tr_shutdown
};

/*
 * func implementations
*/

int __tr_init(void) {
	endpoints_count = 0;
	bzero(endpoints,sizeof(endpoint)*MAX_ENDPOINTS);

	if((udp_socket_fd = socket(PF_INET, SOCK_DGRAM, 0))<0){
		err("cant create udp socket");
	}

	if((nn_socket_fd = nn_socket(AF_SP, NN_REQ))<0){
		err("cant create nn socket");
	}

	return 0;
}

int __tr_add_endpoint(const char* uri) {
	int id = -1;
	struct sockaddr_in sock_addr;
	UriComponents uri_c;
	bool is_udp_proto = false;

	if (endpoints_count == MAX_ENDPOINTS){
		warn("endpoints count limit (%d) reached", MAX_ENDPOINTS);
		return -1;
	}

	// parse uri
	if (parseAddr(uri, &uri_c) == -1) {
		warn("can't parse endpoint '%s'", uri);
		return -1;
	}

	is_udp_proto = (strlen(uri_c.proto) == 0);

	if (is_udp_proto) {
		// fill udp socket address struct
		bzero(&sock_addr, sizeof(sock_addr));
		sock_addr.sin_family = AF_INET;
		sock_addr.sin_port = htons(uri_c.port);
		sock_addr.sin_addr.s_addr = inet_addr(uri_c.host);
	} else {
		// connect via nanomsg
		id = nn_connect(nn_socket_fd, uri);
		if(id < 0){
			warn("can't add endpoint '%s'", uri);
			return -1;
		}
	}

	strcpy(endpoints[endpoints_count].url, uri);
	endpoints[endpoints_count].id = id;
	endpoints[endpoints_count].sock_addr = sock_addr;

	if (endpoints_count == 0) {
		udp_mode = is_udp_proto;
	}

	endpoints_count++;
	return 0;
}

int __tr_get_endpoints_count(void) {
	return endpoints_count;
}

const endpoint *__tr_get_endpoint_at_index(int index) {
	if (index >=0 && index < endpoints_count) {
		return &endpoints[index];
	}

	return NULL;
}

int __tr_set_timeout(long t) {
	if (udp_mode) {
		struct timeval tv;
		tv.tv_sec = t / 1000;
		tv.tv_usec = (t % 1000) * 1000;
		setsockopt(udp_socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	} else {
		nn_setsockopt(nn_socket_fd,NN_SOL_SOCKET,NN_SNDTIMEO,&t,sizeof(int));
		nn_setsockopt(nn_socket_fd,NN_SOL_SOCKET,NN_RCVTIMEO,&t,sizeof(int));
	}

	return 0;
}

int __tr_shut_down_all_connections(void) {
	if (udp_mode == false) {
		for (int i = 0; i<endpoints_count; ++i)
			nn_shutdown(nn_socket_fd, endpoints[i].id);
	}

	return 0;
}

int __tr_remove_all_endpoints(void) {
	bzero(endpoints, sizeof(endpoint)*endpoints_count);
	endpoints_count = 0;
	curr_endpoint_index = 0;
	return 0;
}

int __tr_send_msg(const void *buf, size_t len) {
	int n = -1;

	if (udp_mode) {
		struct sockaddr_in sock_addr;

		for (int i = curr_endpoint_index; i < endpoints_count; ++i) {
			sock_addr = endpoints[i].sock_addr;
			n = sendto(udp_socket_fd, buf, len, 0, (struct sockaddr*)&sock_addr, sizeof(sock_addr));

			if (n < 0 || n != (int)len) {
				dbg("can't send request");

				// is next enpoint available
				if (i + 1 < endpoints_count)
				{
					dbg("try next endpoint");
				}
				else if (curr_endpoint_index > 0)
				{
					curr_endpoint_index = 0;
					i = 0;
					dbg("try first endpoint");
				}

				continue;
			}

			// remember current endpoint index
			curr_endpoint_index = i;
		}
	} else {
		n = nn_send(nn_socket_fd, buf, len, 0);
	}

	return n;
}

int __tr_recv_msg(void *buf, size_t len) {
	int n = -1;

	if (udp_mode) {
		n = recvfrom(udp_socket_fd, buf, len, 0, NULL, NULL);
	} else {
		n = nn_recv(nn_socket_fd, buf, len, 0);
	}

	return n;
}

int __tr_shutdown(void) {
	// udp socket
	shutdown(udp_socket_fd, SHUT_RDWR);
	close(udp_socket_fd);

	// nanomsg
	nn_shutdown(nn_socket_fd,0);
	nn_close(nn_socket_fd);

	return 0;
}
