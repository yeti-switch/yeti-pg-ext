#include "resolver.h"
#include "log.h"
#include "transport.h"
#include "request_id.h"

#include <poll.h>

#define LOG_PREFIX "resolver: "

#define DEFAULT_RTT_TIMEOUT_MSEC 5

#define MSG_SZ 1024 * 2
#define ERROR_TEXT_SZ MSG_SZ

#define CNFRM_RESPONSE_HDR_SIZE 4

#define TAGGED_HDR_SIZE 7
#define TAGGED_ERR_RESPONSE_HDR_SIZE 6
#define TAGGED_RESPONSE_CODE_SUCCESS 0

#define CNAM_HDR_SIZE 10
#define CNAM_RESPONSE_HDR_SIZE 8

#define alloc_text(text, size, fmt, ...)\
	text = (char *)malloc((size + 1) * sizeof(char));\
	snprintf(text, size, fmt, ## __VA_ARGS__);\

#define free_text(text)\
	if (text != NULL) {\
		free(text);\
		text = NULL;\
	}\

#define alloc_error(error, fmt, ...)\
	alloc_text(error, ERROR_TEXT_SZ, fmt, ## __VA_ARGS__);\

#define alloc_error_ptr(error_ptr, fmt, ...)\
	if (error_ptr != NULL) {\
		alloc_error(*error_ptr, fmt, ## __VA_ARGS__);\
	}\

/*
 * local vars
*/

int endpoints_count;
endpoint endpoints[MAX_ENDPOINTS];
int curr_endpoint_index = -1;
int rtt_timeout;

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

int prepare(const request *req, char msg[MSG_SZ], size_t *size, char **error);
int prepare_tagged(const request *req, char msg[MSG_SZ], size_t *size, char **error);
int prepare_json(const request *req, char msg[MSG_SZ], size_t *size, char **error);

int parse(const request *req, const char msg[MSG_SZ], size_t size, response *resp, char **error);
int parse_confrm_resp(const char msg[MSG_SZ], size_t size, response *resp, char **error);
int parse_json(const char msg[MSG_SZ], size_t size, response *resp, char **error);
int parse_tagged(const char msg[MSG_SZ], size_t size, response *resp, char **error);

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
	endpoints_count = 0;
	bzero(endpoints,sizeof(endpoint)*MAX_ENDPOINTS);
	set_rtt_timeout(DEFAULT_RTT_TIMEOUT_MSEC);
	return 0;
}

int add_endpoint(const char* uri) {
	if (endpoints_count == MAX_ENDPOINTS){
		warn("endpoints count limit (%d) reached", MAX_ENDPOINTS);
		return -1;
	}

	strcpy(endpoints[endpoints_count].url, uri);
	endpoints[endpoints_count].id = endpoints_count;
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
	if (curr_endpoint_index < 0) {
		curr_endpoint_index = endpoints_count - 1;
	} else {
		--curr_endpoint_index;
	}

	dbg("get next endpoint index %d", curr_endpoint_index);
	return get_endpoint_at_index(curr_endpoint_index);
}

int remove_all_endpoints(void) {
	bzero(endpoints, sizeof(endpoint)*endpoints_count);
	endpoints_count = 0;
	curr_endpoint_index = -1;
	return 0;
}

int resolve(request *req, response *resp, char **error) {
	size_t size;
	int n = -1;
	const endpoint *endp;
	struct pollfd poll_fd;
	int ready;
	char msg[MSG_SZ];

	if (endpoints_count <= 0) {
		alloc_error_ptr(error, "local: no configured endpoints");
		return -1;
	}

	endp = get_current_endpoint();

	while (endp != NULL) {

		// generate request id
		req->id = gen_request_id();

		// prepare request message
		if (prepare(req, msg, &size, error) != 0) {
			return -1;
		}

		// send message
		n = Transport.send_data(msg, size, endp->url);

		if (n < 0 || n != (int)size) {
			info("can't send request");
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
			memset(&msg, '\0', MSG_SZ);
			n = Transport.recv_data(msg, MSG_SZ);

			if (n < 0) {
				alloc_error_ptr(error, "can't receive response");
				return -1;
			} else {
				msg[n] = '\0';
			}

			// parse message
			memset(resp, 0, sizeof(response));
			if (parse(req, msg, n, resp, error) != 0) {
				return -1;
			}

			if (resp->is_confrm) {
				if (resp->id != req->id) {
					dbg("invalid response id for confirmational message");
				} else {
					dbg("confirm msg is ok");
				}

				// receive full message (go to next iteration with continue)
				continue;
			}

			//check response id
			if (resp->id != req->id) {
				warn("invalid response id");
				continue;
			}

			// full response is available
			return 0;
		}

		break;
	}

	alloc_error_ptr(error, "local: failed to resolve");
	return -1;
}

int set_rtt_timeout(int timeout) {
	rtt_timeout = timeout;
	return 0;
}

int prepare(const request *req, char msg[MSG_SZ], size_t *size, char **error) {
	switch (req->type) {
	case TAGGED_REQ_VERSION:
		return prepare_tagged(req, msg, size, error);
	case CNAM_REQ_VERSION:
		return prepare_json(req, msg, size, error);
	}

	return -1;
}

int prepare_tagged(const request *req, char msg[MSG_SZ], size_t *size, char **error) {
	size_t data_size;

	/* layout:
	 *    4 bytes - request id
	 *    1 byte  - database id
	 *    1 byte  - request version
	 *    1 byte  - data length
	 *    n bytes - data
	 */

	data_size = VARSIZE_ANY_EXHDR(req->data);
	(*size) = data_size + TAGGED_HDR_SIZE;

	if ((*size) > MSG_SZ) {
		alloc_error_ptr(error, "message is to long");
		return -1;
	}

	memset(msg, '\0', MSG_SZ);
	*(uint32_t *)msg = req->id;
	*(uint8_t *)(msg+4) = req->db_id;
	*(uint8_t *)(msg+5) = req->type;
	*(uint8_t *)(msg+6) = data_size;

	memcpy(msg+TAGGED_HDR_SIZE, VARDATA_ANY(req->data), data_size);
	return 0;
}

int prepare_json(const request *req, char msg[MSG_SZ], size_t *size, char **error) {
	size_t data_size;

	/* layout:
	 *    4 bytes - request id
	 *    1 byte  - database id
	 *    1 byte  - request version
	 *    4 byte  - data length
	 *    n bytes - data
	 */

	data_size = VARSIZE_ANY_EXHDR(req->data);
	(*size) = data_size + CNAM_HDR_SIZE;

	if((*size) > MSG_SZ) {
		alloc_error_ptr(error, "message is to long");
		return -1;
	}

	memset(msg, '\0', MSG_SZ);
	*(uint32_t *)msg = req->id;
	*(uint8_t *)(msg+4) = req->db_id;
	*(uint8_t *)(msg+5) = req->type;
	*(uint32_t *)(msg+6) = data_size;

	memcpy(msg+CNAM_HDR_SIZE, VARDATA_ANY(req->data), data_size);
	return 0;
}

int parse(const request *req, const char msg[MSG_SZ], size_t size, response *resp, char **error) {

	if (parse_confrm_resp(msg, size, resp, error) == 0) {
		return 0;
	}

	switch (req->type) {
	case TAGGED_REQ_VERSION:
		return parse_tagged(msg, size, resp, error);
	case CNAM_REQ_VERSION:
		return parse_json(msg, size, resp, error);
	}

	return -1;
}

int parse_confrm_resp(const char msg[MSG_SZ], size_t size, response *resp, char **error) {
	if (size > CNFRM_RESPONSE_HDR_SIZE) {
		return -1;
	}

	/* layout:
	 *    4 bytes - response id
	 */

	//check response layout
	//response must have at least 4 bytes
	if (size < CNFRM_RESPONSE_HDR_SIZE) {
		alloc_error_ptr(error, "local: unexpected response (response too small)");
		return -1;
	}

	resp->id = *(uint32_t *)msg;
	resp->is_confrm = true;
	return 0;
}

int parse_json(const char msg[MSG_SZ], size_t size, response *resp, char **error) {
	uint32_t data_size;

	/* layout:
	 *    4 bytes - response id
	 *    4 bytes - json reply size
	 *    n bytes - json data
	 */

	//check response layout
	//response must have at least 8 bytes
	if (size < CNAM_RESPONSE_HDR_SIZE) {
		alloc_error_ptr(error, "local: unexpected response (response too small)");
		return -1;
	}

	resp->id = *(uint32_t *)msg;
	data_size = *(uint32_t *)(msg+4);

	if (!data_size) {
		alloc_error_ptr(error, "empty reply");
		return -1;
	}

	if (data_size > (size - CNAM_RESPONSE_HDR_SIZE)) {
		alloc_error_ptr(error, "unexpected response "
			"(claimed response length %d but have only %ld bytes at the tail)",
			data_size, size-CNAM_RESPONSE_HDR_SIZE);
		return -1;
	}

	size = data_size + VARHDRSZ;
	resp->val = palloc(size);

	if(!resp->val) {
		alloc_error_ptr(error, "failed to allocate buffer for json data (size: %ld)", size);
		return -1;
	}

	SET_VARSIZE(resp->val, size);
	memcpy(VARDATA(resp->val), msg + CNAM_RESPONSE_HDR_SIZE, data_size);
	return 0;
}

int parse_tagged(const char msg[MSG_SZ], size_t size, response *resp, char **error) {
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
	if (size < TAGGED_HDR_SIZE) {
		alloc_error_ptr(error, "local: unexpected response (response too small)");
		return -1;
	}

	resp->id = *(uint32_t *)msg;
	resp_code = *(uint8_t *)(msg+4);
	data_size = *(uint8_t *)(msg+5);

	dbg("remoute: response code %d", resp_code);
	if (TAGGED_RESPONSE_CODE_SUCCESS != resp_code) {
		if (data_size > (size-TAGGED_ERR_RESPONSE_HDR_SIZE)){
			alloc_error_ptr(error, "local: unexpected response "
				"(claimed response length %d but have only %ld bytes at the tail)",
				data_size, size-TAGGED_ERR_RESPONSE_HDR_SIZE);
			return -1;
		}

		if(data_size) {
			alloc_error_ptr(error, "remote: %d <%.*s>",
				resp_code, data_size, msg+TAGGED_ERR_RESPONSE_HDR_SIZE);
		} else {
			alloc_error_ptr(error, "remote: %d", resp_code);
		}

		return 1;
	}

	if (data_size > (size-TAGGED_HDR_SIZE)){
		alloc_error_ptr(error, "local: unexpected response "
			"(claimed response length %d but have only %ld bytes at the tail)",
			data_size, size-TAGGED_HDR_SIZE);
		return -1;
	}

	lrn_length = *(uint8_t *)(msg+6);
	if (lrn_length > data_size) {
		alloc_error_ptr(error, "local: malformed response "
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
