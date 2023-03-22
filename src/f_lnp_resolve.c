#include "exported_functions.h"
#include "log.h"
#include "transport.h"

#include "funcapi.h"

#include <stdlib.h>

#define LOG_PREFIX "lnp_resolve: "

#define TAGGED_REQ_VERSION 0
#define TAGGED_HDR_SIZE 3
#define TAGGED_ERR_RESPONSE_HDR_SIZE 2
#define TAGGED_RESPONSE_CODE_SUCCESS 0

#define CNAM_REQ_VERSION 1
#define CNAM_HDR_SIZE 6
#define CNAM_RESPONSE_HDR_SIZE 4

#define MSG_SZ 1024 * 2
#define ERROR_TEXT_SZ MSG_SZ

#define goto_exit_with_error(fmt, ...)\
	err_text = (char *)malloc(ERROR_TEXT_SZ);\
	sprintf(err_text, fmt, ## __VA_ARGS__);\
	goto exit;

#define free_error\
	if (err_text != NULL) {\
		free(err_text); err_text = NULL;\
	}


PG_FUNCTION_INFO_V1(lnp_resolve_cnam);
Datum lnp_resolve_cnam(PG_FUNCTION_ARGS)
{
	size_t size, json_data_size;
	int n;
	char msg[MSG_SZ];
	text *ret;

	int32 database_id = PG_GETARG_INT32(0);
	text *json_data = PG_GETARG_TEXT_P(1);

	if(!Transport.get_endpoints_count()) {
		exit_err("no configured endpoints. use lnp_endpoints_set to add endpoint");
	}

	//send request
	json_data_size = VARSIZE_ANY_EXHDR(json_data);
	size = json_data_size + CNAM_HDR_SIZE;

	if(size > MSG_SZ) {
		exit_err("message is to long");
	}

	/* layout:
	*    1 byte - database id
	*    1 byte - request version
	*    4 byte - json_data length
	*    n bytes - json_data
	*/
	memset(&msg, '\0', MSG_SZ);
	msg[0] = database_id;
	msg[1] = CNAM_REQ_VERSION;
	*(int *)(msg+2) = json_data_size;

	memcpy(msg+CNAM_HDR_SIZE,VARDATA_ANY(json_data),json_data_size);
	n = Transport.send_msg(msg, size);

	if(n != (int)size) {
		exit_err("can't send request");
	}

	//receive response
	memset(&msg, '\0', MSG_SZ);
	n = Transport.recv_msg(&msg, MSG_SZ);

	if(n >= 0) {
		msg[n] = '\0';
	} else {
		exit_err("can't get reply");
	}

	/* layout:
	 *    4 bytes  - json_reply_size (n)
	 *    n bytes - json_data
	 */

	//check response layout
	if(n < CNAM_RESPONSE_HDR_SIZE) { //response must have at least 4 bytes
		exit_err("unexpected response (response too small)");
	}

	//check response code
	json_data_size = *(int *)msg;

	//dbg("got cnam response size %ld",json_data_size);
	if(!json_data_size) {
		exit_err("empty reply");
	}

	if(json_data_size > (n - CNAM_RESPONSE_HDR_SIZE)) {
		exit_err("unexpected response "
			"(claimed response length %ld but have only %d bytes at the tail)",
			json_data_size,n-CNAM_RESPONSE_HDR_SIZE);
	}

	size = json_data_size + VARHDRSZ;
	ret = palloc(size);
	if(!ret) {
		exit_err("failed to allocate buffer for json data (size: %ld)", size);
	}

	SET_VARSIZE(ret, size);
	memcpy(VARDATA(ret), msg + CNAM_RESPONSE_HDR_SIZE, json_data_size);
	PG_RETURN_TEXT_P(ret);
}

PG_FUNCTION_INFO_V1(lnp_resolve_tagged);
Datum lnp_resolve_tagged(PG_FUNCTION_ARGS)
{
	size_t l, size, lrn_length, tag_length = 0;
	int ret, response_code;
	HeapTuple t;
	char *v[2];
	char msg[MSG_SZ];
	AttInMetadata *attinmeta;
	TupleDesc tdesc;

	int32 database_id = PG_GETARG_INT32(0);
	text *local_number = PG_GETARG_TEXT_P(1);

	if (get_call_result_type(fcinfo, NULL, &tdesc) != TYPEFUNC_COMPOSITE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context "
					   "that cannot accept type record")));

	if(!Transport.get_endpoints_count()){
		exit_err("no configured endpoints. use lnp_endpoints_set to add endpoint");
	}

	//send request

	l = VARSIZE_ANY_EXHDR(local_number);
	size = l + TAGGED_HDR_SIZE;

	if(size > MSG_SZ) {
		exit_err("message is to long");
	}

	/* layout:
	*    1 byte - database id
	*    1 byte - request version
	*    1 byte - number length
	*    n bytes - number data
	*/
	memset(&msg, '\0', MSG_SZ);
	msg[0] = database_id;
	msg[1] = TAGGED_REQ_VERSION;
	msg[2] = l;
	memcpy(msg+TAGGED_HDR_SIZE,VARDATA_ANY(local_number),l);

	ret = Transport.send_msg(msg, size);

	if(ret!= (int)size) {
		exit_err("can't send request");
	}

	//receive response
	memset(&msg, '\0', MSG_SZ);
	ret = Transport.recv_msg(&msg, MSG_SZ);

	if(ret >= 0) {
		msg[ret] = '\0';
	} else {
		exit_err("can't get reply");
	}

	/* layout:
	 *    1 byte  - response code
	 *    1 byte  - response length
	 *    1 byte  - local routing number length
	 *    n bytes - ported number data
	 *    k bytes - tag data (optional)
	 */

	//check response layout
	if(ret < TAGGED_HDR_SIZE) { //response must have at least 3 bytes
		exit_err("unexpected response (response too small)");
	}

	//check response code
	response_code = msg[0];
	l = msg[1];
	dbg("got response code %d",response_code);
	if(TAGGED_RESPONSE_CODE_SUCCESS!=response_code){
		if(l > (ret-TAGGED_ERR_RESPONSE_HDR_SIZE)){
			exit_err("unexpected response "
				"(claimed response length %ld but have only %d bytes at the tail)",
				l,ret-TAGGED_ERR_RESPONSE_HDR_SIZE);
		}
		if(l) {
			exit_err("got %d <%.*s> from server",
					 response_code,(int)l,msg+TAGGED_ERR_RESPONSE_HDR_SIZE);
		} else {
			exit_err("got %d from server",response_code);
		}
	}

	if(l > (ret-TAGGED_HDR_SIZE)){
		exit_err("unexpected response "
			"(claimed response length %ld but have only %d bytes at the tail)",
			l,ret-TAGGED_HDR_SIZE);
	}

	lrn_length = msg[2];
	if(lrn_length > l) {
		exit_err("malformed response "
			"(claimed lrn_length %ld but total_length is %ld bytes only)",
			lrn_length,l)
	}

	//lrn
	v[0] = palloc((lrn_length+1)*sizeof(char *));
	memcpy(v[0],msg+TAGGED_HDR_SIZE,lrn_length);
	v[0][lrn_length] = 0;

	//tag
	tag_length = l-lrn_length;
	if(l > lrn_length){ //check for the tag prescence in response
		v[1] = palloc((tag_length+1)*sizeof(char *));
		memcpy(v[1],msg+TAGGED_HDR_SIZE+lrn_length,tag_length);
		v[1][tag_length] = 0;
	} else {
		v[1] = NULL;
	}

	attinmeta = TupleDescGetAttInMetadata(tdesc);
	t = BuildTupleFromCStrings(attinmeta, v);

	if(v[1]) pfree(v[1]);
	pfree(v[0]);

	return HeapTupleGetDatum(t);
}

PG_FUNCTION_INFO_V1(lnp_resolve_tagged_with_error);
Datum lnp_resolve_tagged_with_error(PG_FUNCTION_ARGS)
{
	size_t l, size, lrn_length, tag_length, err_length = 0;
	int ret, response_code;
	HeapTuple t;
	char *v[3];
	char *err_text = NULL;
	char msg[MSG_SZ];
	AttInMetadata *attinmeta;
	TupleDesc tdesc;

	int32 database_id = PG_GETARG_INT32(0);
	text *local_number = PG_GETARG_TEXT_P(1);

	if (get_call_result_type(fcinfo, NULL, &tdesc) != TYPEFUNC_COMPOSITE)
		ereport(ERROR,
				(errcode(ERRCODE_FEATURE_NOT_SUPPORTED),
				errmsg("function returning record called in context "
					   "that cannot accept type record")));

	if(!Transport.get_endpoints_count()){
		goto_exit_with_error("no configured endpoints. use lnp_endpoints_set to add endpoint");
	}

	//send request

	l = VARSIZE_ANY_EXHDR(local_number);
	size = l + TAGGED_HDR_SIZE;

	if(size > MSG_SZ) {
		goto_exit_with_error("message is to long");
	}

	/* layout:
	*    1 byte - database id
	*    1 byte - request version
	*    1 byte - number length
	*    n bytes - number data
	*/
	memset(&msg, '\0', MSG_SZ);
	msg[0] = database_id;
	msg[1] = TAGGED_REQ_VERSION;
	msg[2] = l;
	memcpy(msg+TAGGED_HDR_SIZE,VARDATA_ANY(local_number),l);

	ret = Transport.send_msg(msg, size);

	if(ret!= (int)size) {
		goto_exit_with_error("can't send request");
	}

	//receive response
	memset(&msg, '\0', MSG_SZ);
	ret = Transport.recv_msg(&msg, MSG_SZ);

	if(ret >= 0) {
		msg[ret] = '\0';
	} else {
		goto_exit_with_error("can't get reply");
	}

	/* layout:
	 *    1 byte  - response code
	 *    1 byte  - response length
	 *    1 byte  - local routing number length
	 *    n bytes - ported number data
	 *    k bytes - tag data (optional)
	 */

	//check response layout
	if(ret < TAGGED_HDR_SIZE) { //response must have at least 3 bytes
		goto_exit_with_error("unexpected response (response too small)");
	}

	//check response code
	response_code = msg[0];
	l = msg[1];
	dbg("got response code %d",response_code);
	if(TAGGED_RESPONSE_CODE_SUCCESS!=response_code){
		if(l > (ret-TAGGED_ERR_RESPONSE_HDR_SIZE)){
			goto_exit_with_error("unexpected response "
				"(claimed response length %ld but have only %d bytes at the tail",
				l, ret-TAGGED_ERR_RESPONSE_HDR_SIZE);
		}
		if(l) {
			goto_exit_with_error("got %d <%.*s> from server",
				response_code, (int)l, msg+TAGGED_ERR_RESPONSE_HDR_SIZE);

		} else {
			goto_exit_with_error("got %d from server", response_code);
		}
	}

	if(l > (ret-TAGGED_HDR_SIZE)){
		goto_exit_with_error("unexpected response "
			"(claimed response length %ld but have only %d bytes at the tail)",
			l, ret-TAGGED_HDR_SIZE);
	}

	lrn_length = msg[2];
	if(lrn_length > l) {
		goto_exit_with_error("malformed response "
			"(claimed lrn_length %ld but total_length is %ld bytes only)",
			lrn_length, l);
	}

exit:
	if (err_text == NULL) {
		//lrn
		v[0] = palloc((lrn_length+1)*sizeof(char *));
		memcpy(v[0],msg+TAGGED_HDR_SIZE,lrn_length);
		v[0][lrn_length] = 0;

		//tag
		tag_length = l-lrn_length;
		if(l > lrn_length){ //check for the tag prescence in response
			v[1] = palloc((tag_length+1)*sizeof(char *));
			memcpy(v[1],msg+TAGGED_HDR_SIZE+lrn_length,tag_length);
			v[1][tag_length] = 0;
		} else {
			v[1] = NULL;
		}

		//error
		v[2] = NULL;
	} else {
		//lrn
		v[0] = NULL;

		//tag
		v[1] = NULL;

		//error
		err_length = strlen(err_text);
		v[2] = palloc((err_length+1)*sizeof(char *));
		memcpy(v[2],err_text,err_length);
		v[2][err_length] = 0;

		free_error;
	}

	attinmeta = TupleDescGetAttInMetadata(tdesc);
	t = BuildTupleFromCStrings(attinmeta, v);

	if(v[2]) pfree(v[2]);
	if(v[1]) pfree(v[1]);
	if(v[0]) pfree(v[0]);

	return HeapTupleGetDatum(t);
}
