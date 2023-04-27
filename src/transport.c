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
#include <time.h>

#define LOG_PREFIX "transport: "

#define DEFAULT_RECV_TIMEOUT_MSEC 5000

/*
 * local vars
*/

int socket_fd;

/*
 * func prototypes
*/

int __tr_init(void);
int __tr_get_socket_fd(void);
int __tr_set_timeout(long t);
int __tr_send_data(const void *buf, size_t len, const char *url);
int __tr_recv_data(void *buf, size_t len);
int __tr_shutdown(void);

/*
 * 'transport' struct
*/

const struct transport Transport = {
	.init = __tr_init,
	.get_socket_fd = __tr_get_socket_fd,
	.set_timeout = __tr_set_timeout,
	.send_data = __tr_send_data,
	.recv_data = __tr_recv_data,
	.shutdown = __tr_shutdown
};

/*
 * func implementations
*/

int __tr_init(void) {
	if((socket_fd = socket(PF_INET, SOCK_DGRAM, 0))<0){
		err("cant create udp socket");
	}

	__tr_set_timeout(DEFAULT_RECV_TIMEOUT_MSEC);
	srand((unsigned)time(NULL));
	return 0;
}

int __tr_get_socket_fd(void) {
	return socket_fd;
}

int __tr_set_timeout(long t) {
	struct timeval tv;
	tv.tv_sec = t / 1000;
	tv.tv_usec = (t % 1000) * 1000;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	return 0;
}

int __tr_send_data(const void *buf, size_t len, const char *url) {
	int n = -1;
	UriComponents uri_c;
	struct sockaddr_in sock_addr;

	if (parseAddr(url, &uri_c) == -1) {
		warn("can't parse endpoint '%s'", url);
		return -1;
	}

	bzero(&sock_addr, sizeof(sock_addr));
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(uri_c.port);
	sock_addr.sin_addr.s_addr = inet_addr(uri_c.host);

	n = sendto(socket_fd, buf, len, 0,
				(struct sockaddr*)&sock_addr,
				sizeof(struct sockaddr));

	return n;
}

int __tr_recv_data(void *buf, size_t len) {
	int n;
	n = recvfrom(socket_fd, buf, len, 0, NULL, NULL);
	return n;
}

int __tr_shutdown(void) {
	shutdown(socket_fd, SHUT_RDWR);
	close(socket_fd);
	return 0;
}
