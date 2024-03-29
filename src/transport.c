#include "transport.h"
#include "log.h"

#include <stdio.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/time.h>
#include <strings.h>
#include <arpa/inet.h>
#include <utils/array.h>
#include <errno.h>
#include <stdbool.h>
#include <time.h>
#include <string.h>

#define LOG_PREFIX "transport: "

#define DEFAULT_RECV_TIMEOUT_MSEC 5000
#define ERROR_SZ 1024 * 2

#define SET_ERROR(error_ptr, fmt, ...)\
	if (error_ptr != NULL) {\
		memset(tr_error_buf, '\0', sizeof(char)*ERROR_SZ);\
		snprintf(tr_error_buf, ERROR_SZ, fmt, ## __VA_ARGS__);\
		*error_ptr = tr_error_buf;\
	}\

/*
 * local vars
*/

int socket_fd = -1;
char tr_error_buf[ERROR_SZ];

/*
 * func prototypes
*/

int __tr_init(void);
int __tr_init_socket(void);
int __tr_get_socket_fd(void);
int __tr_set_timeout(long t);
int __tr_send_data(const void *buf, size_t len, const struct sockaddr_in *addr, char **error);
int __tr_recv_data(void *buf, size_t len, char **error);
int __tr_shutdown_socket(void);

/*
 * 'transport' struct
*/

const struct transport Transport = {
	.init = __tr_init,
	.init_socket = __tr_init_socket,
	.get_socket_fd = __tr_get_socket_fd,
	.set_timeout = __tr_set_timeout,
	.send_data = __tr_send_data,
	.recv_data = __tr_recv_data,
	.shutdown_socket = __tr_shutdown_socket
};

/*
 * func implementations
*/

int __tr_init(void) {
	srand((unsigned)time(NULL));
	return 0;
}

int __tr_init_socket(void) {
	if (socket_fd >= 0) {
		return -1;
	}

	if((socket_fd = socket(PF_INET, SOCK_DGRAM, 0))<0){
		err("cant create udp socket");
	}

	__tr_set_timeout(DEFAULT_RECV_TIMEOUT_MSEC);
	return 0;
}

int __tr_get_socket_fd(void) {
	return socket_fd;
}

int __tr_set_timeout(long t) {
	struct timeval tv;

	if (socket_fd < 0) {
		return -1;
	}

	tv.tv_sec = t / 1000;
	tv.tv_usec = (t % 1000) * 1000;
	setsockopt(socket_fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	return 0;
}

int __tr_send_data(const void *buf, size_t len, const struct sockaddr_in *addr, char **error) {
	int n;

	if (socket_fd < 0) {
		SET_ERROR(error, "transport: send_data: error: invalid socket_fd");
		return -1;
	}

	n = sendto(
		socket_fd, buf, len, 0,
		(struct sockaddr*)addr,
		sizeof(struct sockaddr));

	if(n < 0) {
		int save_errno = errno;
		if (save_errno)
			SET_ERROR(error, "transport: send_data: errno: %s", strerror(save_errno));
	}

	return n;
}

int __tr_recv_data(void *buf, size_t len, char **error) {
	int n;

	if (socket_fd < 0) {
		SET_ERROR(error, "transport: recv_data: error: invalid socket_fd");
		return -1;
	}

	n = recvfrom(socket_fd, buf, len, 0, NULL, NULL);
	if(n < 0) {
		int save_errno = errno;
		if (save_errno)
			SET_ERROR(error, "transport: recv_data: errno: %s", strerror(save_errno));
	}

	return n;
}

int __tr_shutdown_socket(void) {
	if (socket_fd < 0) {
		return -1;
	}

	shutdown(socket_fd, SHUT_RDWR);
	close(socket_fd);
	socket_fd = -1;
	return 0;
}
