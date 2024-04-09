#include "resolver.h"
#include "log.h"
#include "transport.h"
#include "request_id.h"

#if PGVER >= 1600
#include "varatt.h"
#endif

#include <poll.h>
#include <string.h>
#include <arpa/inet.h>

#define LOG_PREFIX "resolver: "

#define DEFAULT_RTT_TIMEOUT_MSEC 5

#define MSG_SZ 1024 * 2
#define ERROR_SZ MSG_SZ

#define CNFRM_RESPONSE_HDR_SIZE 4

#define TAGGED_HDR_SIZE 7
#define TAGGED_ERR_RESPONSE_HDR_SIZE 6
#define TAGGED_RESPONSE_CODE_SUCCESS 0

#define CNAM_HDR_SIZE 10
#define CNAM_RESPONSE_HDR_SIZE 8

#define SET_ERROR(error_ptr, fmt, ...)\
	if (error_ptr != NULL) {\
		memset(rs_error_buf, '\0', sizeof(char)*ERROR_SZ);\
		snprintf(rs_error_buf, ERROR_SZ, fmt, ## __VA_ARGS__);\
		*error_ptr = rs_error_buf;\
	}\

/*
 * local vars
*/

endpoint endpoints[MAX_ENDPOINTS];
int endpoints_count = 0;
int curr_endpoint_index = 0;

struct pollfd poll_fd;
int rtt_timeout = DEFAULT_RTT_TIMEOUT_MSEC;

char msg[MSG_SZ];
size_t msg_len;
char rs_error_buf[ERROR_SZ];

/*
 * func prototypes
*/

int init(void);

int add_endpoint(const char* uri);
int get_endpoints_count(void);
const endpoint *get_endpoint_at_index(int index);
const endpoint *get_current_endpoint();
const endpoint *get_next_endpoint();
int remove_all_endpoints(void);

int set_rtt_timeout(int timeout);
int resolve(request *req, response *resp, char **error);

int prepare_msg(const request *req, char **error);
int prepare_tagged_msg(const request *req, char **error);
int prepare_json_msg(const request *req, char **error);

int parse_msg(const request *req, response *resp, char **error);
int parse_confrm_resp_msg(response *resp, char **error);
int parse_json_msg(response *resp, char **error);
int parse_tagged_msg(response *resp, char **error);

/*
 * 'resolver' struct
*/

const struct resolver Resolver = {
	.init = init,
	.add_endpoint = add_endpoint,
	.get_endpoints_count = get_endpoints_count,
	.get_endpoint_at_index = get_endpoint_at_index,
	.remove_all_endpoints = remove_all_endpoints,
	.set_rtt_timeout = set_rtt_timeout,
	.resolve = resolve
};

/*
 * func implementations
*/

int init(void) {
	bzero(endpoints,sizeof(endpoint)*MAX_ENDPOINTS);
	return 0;
}

int add_endpoint(const char* uri) {
	endpoint *endp = &endpoints[endpoints_count];
	UriComponents comps;

	if (endpoints_count == MAX_ENDPOINTS){
		warn("endpoints count limit (%d) reached", MAX_ENDPOINTS);
		return -1;
	}

	memset(endp, 0, sizeof(endpoint));

	if (parseAddr(uri, &comps) == -1) {
		warn("can't parse endpoint '%s'", uri);
		return -1;
	}

	endp->id = endpoints_count;
	strncpy(endp->url, uri, MAX_ENDPOINT_LEN);

	endp->addr.sin_family = AF_INET;
	endp->addr.sin_port = htons(comps.port);
	if(0==inet_aton(comps.host, &endp->addr.sin_addr)) {
		warn("invalid host in endpoint '%s'", uri);
		return -1;
	}

	endpoints_count++;

	return 0;
}

int get_endpoints_count(void) {
	return endpoints_count;
}

const endpoint *get_endpoint_at_index(int index) {
	if (index >=0 && index < endpoints_count) {
		return &endpoints[index];
	}

	return NULL;
}

const endpoint *get_current_endpoint() {
	if (curr_endpoint_index >= 0 && curr_endpoint_index < endpoints_count) {
		dbg("get curr endpoint index %d", curr_endpoint_index);
		return get_endpoint_at_index(curr_endpoint_index);
	} else {
		return get_next_endpoint();
	}
}

const endpoint *get_next_endpoint() {
	curr_endpoint_index =
		(curr_endpoint_index + 1) % endpoints_count;

	dbg("get next endpoint index %d", curr_endpoint_index);
	return get_endpoint_at_index(curr_endpoint_index);
}

int remove_all_endpoints(void) {
	bzero(endpoints, sizeof(endpoint)*endpoints_count);
	endpoints_count = 0;
	curr_endpoint_index = 0;
	return 0;
}

int resolve(request *req, response *resp, char **error) {
	int n = -1, ready, endpoints_left;
	const endpoint *endp;

	if (endpoints_count <= 0) {
		SET_ERROR(error, "local: no configured endpoints");
		return -1;
	}

	endp = get_current_endpoint();
	endpoints_left = endpoints_count;

	while (endp != NULL && endpoints_left--) {

		// generate request id
		req->id = gen_request_id();

		// prepare request message
		if (prepare_msg(req, error) != 0) {
			return -1;
		}

		// send request
		dbg("send request: req_id %d", req->id);
		n = Transport.send_data(msg, msg_len, &endp->addr, error);

		if (n < 0 || n != (int)msg_len) {
			if (error != NULL)
				err("can't send request. error: %s", *error);
			else
				err("can't send request");

			endp = get_next_endpoint();
			continue;
		}

		// wait rtt_timeout
		memset(&poll_fd, 0, sizeof(struct pollfd));
		poll_fd.fd = Transport.get_socket_fd();
		poll_fd.events = POLLIN;
		ready = poll(&poll_fd, 1, rtt_timeout);
		dbg("poll rtt ready %d", ready);

		if (ready <= 0 || !(poll_fd.revents & POLLIN)) {
			dbg("rtt expired or failed");
			endp = get_next_endpoint();
			continue;
		}

		// use only this endpoint and don't take next one
		// try to receive all messages
		while (true) {

			// receive message
			memset(&msg, '\0', sizeof(char)*MSG_SZ);
			n = Transport.recv_data(msg, MSG_SZ, error);

			if (n < 0) {
				if (*error == NULL)
					SET_ERROR(error, "can't receive response");

				return -1;
			} else {
				msg_len = n;
				msg[msg_len] = '\0';
			}

			// parse message
			memset(resp, 0, sizeof(response));
			if (parse_msg(req, resp, error) != 0) {
				return -1;
			}

			if (resp->is_confrm) {
				if (resp->id != req->id) {
					warn("invalid resp_id %d for confirmational message, current req_id %d", resp->id, req->id);
				} else {
					dbg("resp_id %d is confirmed, msg is ok", resp->id);
				}

				// receive full message (go to next iteration with continue)
				continue;
			}

			//check response id
			if (resp->id != req->id) {
				warn("invalid resp_id %d, current req_id %d", resp->id, req->id);
				continue;
			}

			// full response is available
			dbg("resp_id %d", resp->id);
			return 0;
		}

		break;
	}

	SET_ERROR(error, "local: failed to resolve");
	return -1;
}

int set_rtt_timeout(int timeout) {
	rtt_timeout = timeout;
	return 0;
}

int prepare_msg(const request *req, char **error) {
	switch (req->type) {
	case TAGGED_REQ_VERSION:
		return prepare_tagged_msg(req, error);
	case CNAM_REQ_VERSION:
		return prepare_json_msg(req, error);
	}

	return -1;
}

int prepare_tagged_msg(const request *req, char **error) {
	size_t data_size;

	/* layout:
	 *    4 bytes - request id
	 *    1 byte  - database id
	 *    1 byte  - request version
	 *    1 byte  - data length
	 *    n bytes - data
	 */

	data_size = VARSIZE_ANY_EXHDR(req->data);
	msg_len = data_size + TAGGED_HDR_SIZE;

	if (msg_len > MSG_SZ) {
		SET_ERROR(error, "message is to long");
		return -1;
	}

	memset(msg, '\0', sizeof(char)*MSG_SZ);
	*(uint32_t *)msg = req->id;
	*(uint8_t *)(msg+4) = req->db_id;
	*(uint8_t *)(msg+5) = req->type;
	*(uint8_t *)(msg+6) = data_size;

	memcpy(msg+TAGGED_HDR_SIZE, VARDATA_ANY(req->data), data_size);
	return 0;
}

int prepare_json_msg(const request *req, char **error) {
	size_t data_size;

	/* layout:
	 *    4 bytes - request id
	 *    1 byte  - database id
	 *    1 byte  - request version
	 *    4 byte  - data length
	 *    n bytes - data
	 */

	data_size = VARSIZE_ANY_EXHDR(req->data);
	msg_len = data_size + CNAM_HDR_SIZE;

	if(msg_len > MSG_SZ) {
		SET_ERROR(error, "message is to long");
		return -1;
	}

	memset(msg, '\0', sizeof(char)*MSG_SZ);
	*(uint32_t *)msg = req->id;
	*(uint8_t *)(msg+4) = req->db_id;
	*(uint8_t *)(msg+5) = req->type;
	*(uint32_t *)(msg+6) = data_size;

	memcpy(msg+CNAM_HDR_SIZE, VARDATA_ANY(req->data), data_size);
	return 0;
}

int parse_msg(const request *req, response *resp, char **error) {

	if (parse_confrm_resp_msg(resp, error) == 0) {
		return 0;
	}

	switch (req->type) {
	case TAGGED_REQ_VERSION:
		return parse_tagged_msg(resp, error);
	case CNAM_REQ_VERSION:
		return parse_json_msg(resp, error);
	}

	return -1;
}

int parse_confrm_resp_msg(response *resp, char **error) {
	if (msg_len > CNFRM_RESPONSE_HDR_SIZE) {
		return -1;
	}

	/* layout:
	 *    4 bytes - response id
	 */

	//check response layout
	//response must have at least 4 bytes
	if (msg_len < CNFRM_RESPONSE_HDR_SIZE) {
		SET_ERROR(error, "local: unexpected response (response too small)");
		return -1;
	}

	resp->id = *(uint32_t *)msg;
	resp->is_confrm = true;
	return 0;
}

int parse_json_msg(response *resp, char **error) {
	uint32_t data_size;
	size_t size;

	/* layout:
	 *    4 bytes - response id
	 *    4 bytes - json reply size
	 *    n bytes - json data
	 */

	//check response layout
	//response must have at least 8 bytes
	if (msg_len < CNAM_RESPONSE_HDR_SIZE) {
		SET_ERROR(error, "local: unexpected response (response too small)");
		return -1;
	}

	resp->id = *(uint32_t *)msg;
	data_size = *(uint32_t *)(msg+4);

	if (!data_size) {
		SET_ERROR(error, "empty reply");
		return -1;
	}

	if (data_size > (msg_len - CNAM_RESPONSE_HDR_SIZE)) {
		SET_ERROR(error, "unexpected response "
			"(claimed response length %d but have only %ld bytes at the tail)",
			data_size, msg_len-CNAM_RESPONSE_HDR_SIZE);
		return -1;
	}

	size = data_size + VARHDRSZ;
	resp->val = palloc(size);

	if(!resp->val) {
		SET_ERROR(error, "failed to allocate buffer for json data (size: %ld)", size);
		return -1;
	}

	SET_VARSIZE(resp->val, size);
	memcpy(VARDATA(resp->val), msg + CNAM_RESPONSE_HDR_SIZE, data_size);
	return 0;
}

int parse_tagged_msg(response *resp, char **error) {
	uint8_t data_size, lrn_length, tag_length, resp_code;

	/* layout:
	 *    4 bytes - response id
	 *    1 byte  - response code
	 *    1 byte  - response length
	 *    1 byte  - local routing number length
	 *    n bytes - ported number data
	 *    k bytes - tag data (optional)
	 */

	//check response layout
	//response must have at least 7 bytes
	if (msg_len < TAGGED_HDR_SIZE) {
		SET_ERROR(error, "local: unexpected response (response too small)");
		return -1;
	}

	resp->id = *(uint32_t *)msg;
	resp_code = *(uint8_t *)(msg+4);
	data_size = *(uint8_t *)(msg+5);

	dbg("remote: response code %d", resp_code);
	if (TAGGED_RESPONSE_CODE_SUCCESS != resp_code) {
		if (data_size > (msg_len-TAGGED_ERR_RESPONSE_HDR_SIZE)){
			SET_ERROR(error, "local: unexpected response "
				"(claimed response length %d but have only %ld bytes at the tail)",
				data_size, msg_len-TAGGED_ERR_RESPONSE_HDR_SIZE);
			return -1;
		}

		if(data_size) {
			SET_ERROR(error, "remote: %d <%.*s>",
				resp_code, data_size, msg+TAGGED_ERR_RESPONSE_HDR_SIZE);
		} else {
			SET_ERROR(error, "remote: %d", resp_code);
		}

		return 1;
	}

	if (data_size > (msg_len-TAGGED_HDR_SIZE)){
		SET_ERROR(error, "local: unexpected response "
			"(claimed response length %d but have only %ld bytes at the tail)",
			data_size, msg_len-TAGGED_HDR_SIZE);
		return -1;
	}

	lrn_length = *(uint8_t *)(msg+6);
	if (lrn_length > data_size) {
		SET_ERROR(error, "local: malformed response "
			"(claimed lrn_length %d but total_length is %d bytes only)",
			lrn_length, data_size);
		return -1;
	}

	// lrn
	resp->val_1 = palloc((lrn_length+1)*sizeof(char));
	memcpy(resp->val_1, msg+TAGGED_HDR_SIZE, lrn_length);
	resp->val_1[lrn_length] = 0;

	//tag
	tag_length = data_size - lrn_length;
	if (tag_length > 0) {
		resp->val_2 = palloc((tag_length+1)*sizeof(char));
		memcpy(resp->val_2, msg+TAGGED_HDR_SIZE+lrn_length, tag_length);
		resp->val_2[tag_length] = 0;
	} else {
		resp->val_2 = NULL;
	}

	return 0;
}
