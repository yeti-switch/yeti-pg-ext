#include "exported_functions.h"
#include "windowapi.h"
#include "log.h"

#include <sys/time.h>

#define LOG_PREFIX "rank_dns_srv(): "

#define CTX_MAX_ROWS 20

typedef struct {
	int32 rank, w, w_sum;
} row_stat;

typedef row_stat rank_dns_srv_ctx[CTX_MAX_ROWS];

static inline void fill_weights(WindowObject w, rank_dns_srv_ctx *ctx, int64 rows)
{
	int64 row;
	int32 weight;
	bool is_null, is_out;
	row_stat *s = *ctx;

	for(row = 0; row < rows; row++, s++) {
		weight = DatumGetInt32(WinGetFuncArgInPartition(
			w,0,row,WINDOW_SEEK_HEAD,false,&is_null,&is_out));
		 s->w = is_null ? 0 : weight;
	}
}

static inline int64 get_next_row(rank_dns_srv_ctx *ctx, int64 rows, unsigned int *seed)
{
	int32 r;
	int64 row, sum;
	row_stat *s;

	sum = 0;
	s = *ctx;
	for(row = 0; row < rows; row++, s++) {
		if(s->rank) continue;
		sum+=s->w;
		s->w_sum = sum;
		//dbg("get_next_row() row = %ld, w = %d, sum = %ld",row,s->w,sum);
	}

	r = rand_r(seed)%(sum+1);
	s = *ctx;
	for(row = 0; row < rows; row++, s++) {
		if(s->rank) continue;
		if(s->w_sum >= r) {
			//info("condition matched: %d > %d for row %ld", s->w_sum,r,row);
			break;
		}
	}
	return row;
}

static inline void fill_distribution(WindowObject w, rank_dns_srv_ctx *ctx)
{
	int32 rank;
	int64 row, rows;
	struct timeval tv;
	unsigned int seed;

	rows = WinGetPartitionRowCount(w);
	if(rows > CTX_MAX_ROWS) row = CTX_MAX_ROWS;

	fill_weights(w,ctx,rows);

	gettimeofday(&tv,NULL);
	seed = tv.tv_usec;

	rank = 0;
	while(rank < rows) {
		row = get_next_row(ctx,rows,&seed);
		rank++;
		(*ctx)[row].rank = rank;
	}
}

PG_FUNCTION_INFO_V1(rank_dns_srv);
Datum rank_dns_srv(PG_FUNCTION_ARGS)
{
	int64 row;
	rank_dns_srv_ctx *ctx;
	WindowObject w = PG_WINDOW_OBJECT();

	ctx = (rank_dns_srv_ctx *)WinGetPartitionLocalMemory(w, sizeof(rank_dns_srv_ctx));
	row = WinGetCurrentPosition(w);

	if(0==row) fill_distribution(w,ctx);

	if(row >= CTX_MAX_ROWS) {
		PG_RETURN_INT32(CTX_MAX_ROWS+1);
	}
	PG_RETURN_INT32((*ctx)[row].rank);
}

