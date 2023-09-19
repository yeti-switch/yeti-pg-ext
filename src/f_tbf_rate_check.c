#include "f_tbf_rate_check.h"
#include "exported_functions.h"

#include "postgres.h"
#include "miscadmin.h"
#include "storage/shmem.h"
#include "storage/lwlock.h"
#include "storage/ipc.h"
#include "portability/instr_time.h"

#define TBF_DEBUG 0
#define TBF_HASH_MAX_ENTRIES 5000

#if TBF_DEBUG
#define tbf_log(fmt, ...) elog(INFO,fmt, ## __VA_ARGS__)
#else
#define tbf_log(fmt, ...)
#endif

typedef struct tbfSharedState
{
    LWLock *lock;
} tbfSharedState;

typedef struct tbfHashKey
{
    int32 namespace_id;
    int64 bucket_id;
} tbfHashKey;

typedef struct tbfHashEntry
{
    tbfHashKey key;

    instr_time updated_at;  //bucket modification time
    Size tokens;            //actual tokens count within the bucket
} tbfHashEntry;

const char *tbf_shmem_entry_name = "yeti_pg_ext.tbf_shmem";
const char *tbf_shmem_hash_entry_name = "yeti_pg_ext.tbf_shmem hash";

static HTAB *tbf_hash = NULL;
static int tbf_hash_max = TBF_HASH_MAX_ENTRIES;
static tbfSharedState *tbf = NULL;

static shmem_request_hook_type prev_shmem_request_hook = NULL;
static shmem_startup_hook_type prev_shmem_startup_hook = NULL;

static Size tbf_shmem_size(void);
static void tbf_shmem_startup(void);

void tbf_init(void);
void tbf_fini(void);

Size tbf_shmem_size(void)
{
    Size size;

    size = MAXALIGN(sizeof(tbfSharedState));
    size = add_size(size, hash_estimate_size(tbf_hash_max, sizeof(tbfHashEntry)));

    return size;
}

static void tbf_shmem_request(void)
{
    if(prev_shmem_request_hook)
        prev_shmem_request_hook();

    RequestAddinShmemSpace(tbf_shmem_size());
    RequestNamedLWLockTranche(tbf_shmem_entry_name, 1);
}

void tbf_shmem_startup(void)
{
    bool found;
    HASHCTL info;

    if(prev_shmem_startup_hook)
        prev_shmem_startup_hook();

    tbf = NULL;
    tbf_hash = NULL;

    LWLockAcquire(AddinShmemInitLock, LW_EXCLUSIVE);

    tbf = ShmemInitStruct(tbf_shmem_entry_name,
                            sizeof(tbfSharedState),
                            &found);

    if(!found) {
        tbf->lock = &(GetNamedLWLockTranche(tbf_shmem_entry_name))->lock;
    }

    memset(&info, 0, sizeof(info));
    info.keysize = sizeof(tbfHashKey);
    info.entrysize = sizeof(tbfHashEntry);
    tbf_hash = ShmemInitHash(tbf_shmem_hash_entry_name,
                             tbf_hash_max, tbf_hash_max,
                             &info,
                             HASH_ELEM | HASH_BLOBS);

    LWLockRelease(AddinShmemInitLock);
}

void tbf_init(void)
{
    if(!process_shared_preload_libraries_in_progress)
        return;

    prev_shmem_request_hook = shmem_request_hook;
    shmem_request_hook = tbf_shmem_request;

    prev_shmem_startup_hook = shmem_startup_hook;
    shmem_startup_hook = tbf_shmem_startup;
}

void tbf_fini(void)
{
    if(tbf) {
        shmem_request_hook = prev_shmem_request_hook;
        shmem_startup_hook = prev_shmem_startup_hook;
    }
}

PG_FUNCTION_INFO_V1(tbf_rate_check);
Datum tbf_rate_check(PG_FUNCTION_ARGS)
{
    enum args_idx {
        ARG_NAMESPACE_ID = 0,
        ARG_BUCKET_ID,
        ARG_RATE
    };

    tbfHashKey key;
    tbfHashEntry *entry;
    float4 rate;
    bool found, ret;
    instr_time now, diff;
    Size bucket_max_size;

    if(!tbf) {
        elog(WARNING, "shared memory dependent 'tbf_rate_check' called "
                      "while extension is not listed in 'shared_preload_libraries'. "
                      "return true");
        PG_RETURN_BOOL(true);
    }

    if(PG_ARGISNULL(ARG_NAMESPACE_ID)
       || PG_ARGISNULL(ARG_BUCKET_ID)
       || PG_ARGISNULL(ARG_RATE))
    {
        PG_RETURN_BOOL(true);
    }

    rate = PG_GETARG_FLOAT4(ARG_RATE);
    if(!rate) {
        //always false fort zero rate
        PG_RETURN_BOOL(false);
    }

    key.namespace_id = PG_GETARG_UINT32(ARG_NAMESPACE_ID);
    key.bucket_id = PG_GETARG_INT64(ARG_BUCKET_ID);

    INSTR_TIME_SET_CURRENT(now);

    bucket_max_size = rate*2;
    if(bucket_max_size < 1) bucket_max_size = 1;

    tbf_log("tbf_rate_check(namespace_id:%u,bucket_id:%lu,rate:%f) now: %f, bucket_max_size:%ld",
        key.namespace_id, key.bucket_id, rate,
        INSTR_TIME_GET_DOUBLE(now), bucket_max_size);

    LWLockAcquire(tbf->lock, LW_EXCLUSIVE);

    entry = (tbfHashEntry *)hash_search(tbf_hash, &key, HASH_ENTER, &found);
    if(!found) {
        entry->updated_at = now;
        entry->tokens = bucket_max_size-1;
        tbf_log("new entry: updated_at:%f tokens:%ld",
                INSTR_TIME_GET_DOUBLE(entry->updated_at), entry->tokens);
        //always pass initial call
        ret = true;
    } else {
        tbf_log("found entry: updated_at:%f tokens:%ld",
             INSTR_TIME_GET_DOUBLE(entry->updated_at), entry->tokens);

        diff = now;
        INSTR_TIME_SUBTRACT(diff, entry->updated_at);

        tbf_log("update time diff: %f", INSTR_TIME_GET_DOUBLE(diff));

        //add tokens using update time diff
        entry->tokens += rate*INSTR_TIME_GET_DOUBLE(diff);
        tbf_log("tokens after the addition: %ld", entry->tokens);

        if(entry->tokens > 0) {
            entry->updated_at = now;

            if(entry->tokens > bucket_max_size)
                entry->tokens = bucket_max_size;
            tbf_log("tokens after the bucket_max_size limitation: %ld", entry->tokens);

            entry->tokens--;
            tbf_log("tokens after the substraction: %ld", entry->tokens);

            ret = true;
        } else {
            ret = false;
        }
    }

    LWLockRelease(tbf->lock);

    PG_RETURN_BOOL(ret);
}
