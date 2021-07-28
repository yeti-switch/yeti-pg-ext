#include "exported_functions.h"
#include "log.h"
#include "shared_vars.h"

#include "funcapi.h"

#include <stdlib.h>

#include "sys/errno.h"
#include <nanomsg/nn.h>
#include <nanomsg/reqrep.h>

#define LOG_PREFIX "lnp_resolve: "

#define nn_error(fmt, ...) exit_err(fmt, ## __VA_ARGS__)

#define OLD_PDU_HDR_SIZE 2
#define PDU_HDR_SIZE 3
#define ERR_RESPONSE_HDR_SIZE 2
#define REQ_VERSION 0
#define RESPONSE_CODE_SUCCESS 0

PG_FUNCTION_INFO_V1(lnp_resolve_tagged);
Datum lnp_resolve_tagged(PG_FUNCTION_ARGS)
{
	size_t l, size, lrn_length, tag_length = 0;
	int ret, response_code;
	HeapTuple t;
	char *v[2], *msg;
	AttInMetadata *attinmeta;
	TupleDesc tdesc;

	int32 database_id = PG_GETARG_INT32(0);
	text *local_number = PG_GETARG_TEXT_P(1);


	if (get_call_result_type(fcinfo, NULL, &tdesc) != TYPEFUNC_COMPOSITE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context "
					   "that cannot accept type record")));

	if(!endpoints_count){
		exit_err("no configured endpoints. use lnp_endpoints_set to add endpoint");
	}

	//send request

	l = VARSIZE_ANY_EXHDR(local_number);
	size = l + PDU_HDR_SIZE;

	msg = (char *)malloc(size);
	if(NULL==msg){
		//nn_close(s);
		exit_err("can't allocate memory for request buffer");
	}

	/* layout:
	*    1 byte - database id
	*    1 byte - request version
	*    1 byte - number length
	*    n bytes - number data
	*/
	msg[0] = database_id;
	msg[1] = REQ_VERSION;
	msg[2] = l;
	memcpy(msg+PDU_HDR_SIZE,VARDATA_ANY(local_number),l);

	ret = nn_send(nn_socket_fd, msg, size, 0);
	free(msg);
	if(ret!=size){
		nn_error("can't send request");
	}

	//receive response
	msg = NULL;
	ret = nn_recv(nn_socket_fd,&msg,NN_MSG,0);
	if(ret < 0){
		nn_error("can't get reply");
	}

	/* layout:
	 *    1 byte  - response code
	 *    1 byte  - response length
	 *    1 byte  - local routing number length
	 *    n bytes - ported number data
	 *    k bytes - tag data (optional)
	 */

	//check response layout
	if(ret < PDU_HDR_SIZE) { //response must have at least 3 bytes
		nn_freemsg(msg);
		exit_err("unexpected response (response too small)");
	}

	//check response code
	response_code = msg[0];
	l = msg[1];
	dbg("got response code %d",response_code);
	if(RESPONSE_CODE_SUCCESS!=response_code){
		if(l > (ret-ERR_RESPONSE_HDR_SIZE)){
			nn_freemsg(msg);
			exit_err("unexpected response "
				"(claimed response length %ld but have only %d bytes at the tail)",
				l,ret-ERR_RESPONSE_HDR_SIZE);
		}
		if(l) {
			nn_freemsg(msg);
			exit_err("got %d <%.*s> from server",response_code,(int)l,msg+ERR_RESPONSE_HDR_SIZE);
		} else {
			nn_freemsg(msg);
			exit_err("got %d from server",response_code);
		}
	}

	if(l > (ret-PDU_HDR_SIZE)){
		nn_freemsg(msg);
		exit_err("unexpected response "
			"(claimed response length %ld but have only %d bytes at the tail)",
			l,ret-PDU_HDR_SIZE);
	}

	lrn_length = msg[2];
	if(lrn_length > l) {
		nn_freemsg(msg);
		exit_err("malformed response "
			"(claimed lrn_length %ld but total_length is %ld bytes only)",
			lrn_length,l)
	}

	//lrn
	v[0] = palloc((lrn_length+1)*sizeof(char *));
	memcpy(v[0],msg+PDU_HDR_SIZE,lrn_length);
	v[0][lrn_length] = 0;

	//tag
	tag_length = l-lrn_length;
	if(l > lrn_length){ //check for the tag prescence in response
		v[1] = palloc((tag_length+1)*sizeof(char *));
		memcpy(v[1],msg+PDU_HDR_SIZE+lrn_length,tag_length);
		v[1][tag_length] = 0;
	} else {
		v[1] = NULL;
	}

	nn_freemsg(msg);

	attinmeta = TupleDescGetAttInMetadata(tdesc);
	t = BuildTupleFromCStrings(attinmeta, v);

	if(v[1]) pfree(v[1]);
	pfree(v[0]);

	return HeapTupleGetDatum(t);
}

PG_FUNCTION_INFO_V1(lnp_resolve);
Datum lnp_resolve(PG_FUNCTION_ARGS)
{
	size_t l, size;
	text *out;
	char *msg;
	int ret, response_code;

	int32 database_id = PG_GETARG_INT32(0);
	text *local_number = PG_GETARG_TEXT_P(1);

	if(!endpoints_count){
		exit_err("no configured endpoints. use lnp_endpoints_set to add endpoint");
	}

	//send request

	l = VARSIZE_ANY_EXHDR(local_number);
	size = l + OLD_PDU_HDR_SIZE;

	msg = (char *)malloc(size);
	if(NULL==msg){
		//nn_close(s);
		exit_err("can't allocate memory for request buffer");
	}

	/* layout:
	*    1 byte - database id
	*    1 byte - local number length
	*    n bytes - local number data
	*/
	msg[0] = database_id;
	msg[1] = l;
	memcpy(msg+OLD_PDU_HDR_SIZE,VARDATA_ANY(local_number),l);

	ret = nn_send(nn_socket_fd, msg, size, 0);
	free(msg);
	if(ret!=size){
		nn_error("can't send request");
	}

	//receive response
	msg = NULL;
	ret = nn_recv(nn_socket_fd,&msg,NN_MSG,0);
	if(ret < 0){
		nn_error("can't get reply");
	}

	/* layout:
	 *    1 byte - response code
	 *    1 byte - response length
	 *    n bytes - response data
	 */

	//check response layout
	if(ret < OLD_PDU_HDR_SIZE) { //response must have at least 2 bytes
		nn_freemsg(msg);
		exit_err("unexpected response (response too small)");
	}

	l = msg[1];
	if(l > (ret-OLD_PDU_HDR_SIZE)){
		nn_freemsg(msg);
		exit_err("unexpected response "
			"(claimed response length %ld but have only %d bytes at the tail)",
			l,ret-OLD_PDU_HDR_SIZE);
	}

	//check response code
	response_code = msg[0];
	dbg("got response code %d",response_code);
	if(RESPONSE_CODE_SUCCESS!=response_code){
		if(l) {
			nn_freemsg(msg);
			exit_err("got %d <%.*s> from server",response_code,(int)l,msg+ERR_RESPONSE_HDR_SIZE);
		} else {
			nn_freemsg(msg);
			exit_err("got %d from server",response_code);
		}
	}

	//copy response to the output
	size = l + VARHDRSZ;
	out = (text *)palloc(size);
	SET_VARSIZE(out, size);
	memcpy(VARDATA_ANY(out),msg+OLD_PDU_HDR_SIZE,l);

	//cleanup
	nn_freemsg(msg);

	PG_RETURN_TEXT_P(out);
}
