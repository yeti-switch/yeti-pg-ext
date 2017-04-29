#include "replace_rand.h"

#include "log.h"

#include <stdlib.h>
#include <sys/time.h>

#define RAND_START "r("
#define RAND_START_LEN 2
#define RAND_END   ')'

#define RAND_MIN_LEN 0
#define RAND_MAX_LEN 64

#define CACHE_SIZE_INC_START 4
#define CACHE_SIZE_INC_SCALE 2
#define CACHE_SIZE_INC_MAX   64

#define LOG_PREFIX "replace_rand: "

typedef struct {
	const char *start; //placholder start position
	const char *end; //placholder end position
	long value; //parsed value
} replacing;

text *replace(text *in, bool *replaced){

	size_t	i,k,ret_len, s_len,
			n = 0,
			c_size = 0,
			c_size_inc = CACHE_SIZE_INC_START;

	long value;
	unsigned int seed;
	struct timeval tv;

	replacing *c = NULL,*ce;
	text *ret = NULL;
	char *ret_ptr;
	const char	*p3, *p2,*p,*s,*s_end;

	p = s = (char *)VARDATA(in);
	s_len = VARSIZE(in) - VARHDRSZ;
	s_end = s+s_len;

	//find all placeholders. store them to dynamic cache
	while((p2 = strstr(p, RAND_START))!=NULL){
		p3 = strchr(p2+1,RAND_END);
		if(NULL==p3){
			dbg("no closing bracket for placeholder started at position %ld. skip",p2-p);
			p = p2+RAND_START_LEN;
			if(p > s_end) break;
			continue;
		}

		p = p3;

		if(1!=sscanf(p2+RAND_START_LEN,"%ld",&value)){
			dbg("can't parse placeholder argument '%.*s'. skip",(int)(p3-p2-RAND_START_LEN),p2+RAND_START_LEN);
			continue;
		}

		if(value < RAND_MIN_LEN){
			dbg("random string len(%ld) is less then min(%d). set it to %d",
				value,RAND_MIN_LEN,RAND_MIN_LEN);
			value = RAND_MIN_LEN;
		} else if(value > RAND_MAX_LEN){
			dbg("random string len(%ld) is greater then max(%d). set it to %d",
				value,RAND_MAX_LEN,RAND_MAX_LEN);
			value = RAND_MAX_LEN;
		}

		n++;

		if(c_size<n){
			c_size+=c_size_inc;
			c = realloc(c, sizeof(*c) * c_size);
			if(NULL==c){
				dbg("no space for internal buffer");
				goto out;
			}
			//scale increment
			c_size_inc*=CACHE_SIZE_INC_SCALE;
			if(c_size_inc > CACHE_SIZE_INC_MAX){
				c_size_inc = CACHE_SIZE_INC_MAX;
			}
		}

		ce = c+n-1;
		ce->start = p2;
		ce->end = p3;
		ce->value = value;

	}

	if(0==n){
		ret = in;
		*replaced = false;
		goto out;
	}

	*replaced = true;

	//calc output size
	ret_len = s_len;
	ce = c;
	for(i=0;i<n;i++,ce++){
		ret_len-=(ce->end-ce->start)+1; //placeholder length
		ret_len+=ce->value;				//value length
	}

	ret = (text *)palloc(ret_len + VARHDRSZ);
	if(NULL==ret) {
		dbg("no space for output buffer");
		goto out;
	}
	SET_VARSIZE(ret,ret_len+VARHDRSZ);
	ret_ptr = VARDATA(ret);

	ce = c;
	p = s;

	gettimeofday(&tv,NULL);
	seed = tv.tv_usec;
	for(i=0;i<n;i++,ce++){
		memcpy(ret_ptr,p,ce->start-p);
		ret_ptr += ce->start-p;

		for(k=0;k<ce->value;k++)
			*ret_ptr++ = 0x30+rand_r(&seed)%10;

		p = ce->end+1;
	}
	memcpy(ret_ptr,p,s_end-p);
out:
	free(c);
	return ret;
}

Datum get_in_copy(PG_FUNCTION_ARGS){
	text *in,*t;
	in = PG_GETARG_TEXT_P(0);
	t = (text *)palloc(VARSIZE(in));
	if(!t){
		warn("no memory");
		fcinfo->isnull = true;
		return (Datum)0;
	}
	SET_VARSIZE(t, VARSIZE(in));
	memcpy(VARDATA(t),VARDATA(in),VARSIZE(in)-VARHDRSZ);
	return PointerGetDatum(t);
}
