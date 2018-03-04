#include "exported_functions.h"
#include "log.h"
#include "utils/array.h"
#include "catalog/pg_type.h"

#define LOG_PREFIX "tag_compare(): "

#if PGVER > 904
#define GET_ARRAY_ITERATOR(a) array_create_iterator(a,0,NULL);
#else
#define GET_ARRAY_ITERATOR(a) array_create_iterator(a,0);
#endif

inline static bool set_contains_value(Datum *s, int end_pos, Datum value) {
    while(end_pos >= 0) {
        if(DatumGetInt32(s[end_pos])==DatumGetInt32(value))
            return true;
        end_pos--;
    }
    return false;
}

//tag_compare(a int[], b int[])
PG_FUNCTION_INFO_V1(tag_compare);
Datum tag_compare(PG_FUNCTION_ARGS)
{
    enum args_idx {
        ARG_A = 0,
        ARG_B
    };

    enum result_code {
        RET_NOT_MATCHED = 0,
        RET_MATCHED_WITH_NULL,
        RET_MATCHED_WITHOUT_NULL,
        RET_MATCHED_EXACTLY
    };

    ArrayType *a, *b;
    ArrayIterator it;
    Datum *aset, *bset, v;

    bool is_null, a_has_null;
    int ret, apos, bpos, a_len, b_len;

    if(PG_ARGISNULL(ARG_A))
        PG_RETURN_INT32(RET_NOT_MATCHED);

    if(PG_ARGISNULL(ARG_B))
        PG_RETURN_INT32(RET_NOT_MATCHED);

    a = PG_GETARG_ARRAYTYPE_P(ARG_A);
    a_len = ArrayGetNItems(ARR_NDIM(a), ARR_DIMS(a));

    b = PG_GETARG_ARRAYTYPE_P(ARG_B);
    b_len = ArrayGetNItems(ARR_NDIM(b), ARR_DIMS(b));

    if(0==a_len) {
        if(0==b_len)
            PG_RETURN_INT32(RET_MATCHED_EXACTLY);
        PG_RETURN_INT32(RET_NOT_MATCHED);
    }
    if(0==b_len)
        PG_RETURN_INT32(RET_NOT_MATCHED);

    aset = (Datum *)palloc(a_len * sizeof(Datum));

    it = GET_ARRAY_ITERATOR(a);
    apos = -1;
    a_has_null = false;
    while(array_iterate(it,&v,&is_null)) {
        if(is_null) {
            a_has_null |= is_null;
            continue;
        }
        if(set_contains_value(aset,apos,v)) continue;
        aset[++apos] = v;
    }
    //array_free_iterator(it);

    bset = (Datum *)palloc(b_len * sizeof(Datum));

    bpos = -1;
    ret = RET_MATCHED_WITHOUT_NULL;
    it = GET_ARRAY_ITERATOR(b);
    while(array_iterate(it,&v,&is_null)) {
        if(is_null) continue;
        if(!set_contains_value(aset,apos,v)) {
            if(a_has_null) {
                ret = RET_MATCHED_WITH_NULL;
                break;
            }
            ret = RET_NOT_MATCHED;
            break;
        }
        if(!set_contains_value(bset,bpos,v))
            bset[++bpos] = v;
    }

    pfree(aset);
    pfree(bset);
    //array_free_iterator(it);

    if(ret==RET_MATCHED_WITHOUT_NULL && apos==bpos) {
        ret = RET_MATCHED_EXACTLY;
    }

    PG_RETURN_INT32(ret);
}
