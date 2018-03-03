#include "exported_functions.h"
#include "log.h"
#include "utils/array.h"
#include "catalog/pg_type.h"
#include "utils/typcache.h"

#define LOG_PREFIX "tag_action(): "

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

//tag_action(op int, a int[], b int[])
PG_FUNCTION_INFO_V1(tag_action);
Datum tag_action(PG_FUNCTION_ARGS)
{
    enum args_idx {
        ARG_OP = 0,
        ARG_A,
        ARG_B
    };

    enum opts {
        OP_CLEAR = 1,
        OP_REMOVE,
        OP_APPEND,
        OP_INTERSECTION
    };

    bool is_null;
    int apos, bpos, rpos, i, op, a_len, b_len;

    ArrayIterator it;
    ArrayType *r, *a, *b;
    Datum *aset, *bset, *rset, v;

    op = PG_GETARG_INT32(ARG_OP);

    switch(op) {

    case OP_CLEAR:

        PG_RETURN_ARRAYTYPE_P((construct_empty_array(INT4OID)));

    case OP_REMOVE:
    case OP_APPEND:
    case OP_INTERSECTION:

        a = PG_GETARG_ARRAYTYPE_P(ARG_A);
        a_len = ArrayGetNItems(ARR_NDIM(a), ARR_DIMS(a));

        b = PG_GETARG_ARRAYTYPE_P(ARG_B);
        b_len = ArrayGetNItems(ARR_NDIM(b), ARR_DIMS(b));

        aset = (Datum *)palloc(a_len * sizeof(Datum));

        //fill aset with ptrs to unique values from ARG_A
        it = GET_ARRAY_ITERATOR(a);
        apos = -1;
        while(array_iterate(it,&v,&is_null)) {
            if(is_null) continue;
            if(set_contains_value(aset,apos,v)) continue;
            aset[++apos] = v;
        }
        array_free_iterator(it);
        break;

    default:
        ereport(ERROR,
                (errcode(ERRCODE_DATA_EXCEPTION),
                errmsg(LOG_PREFIX "unknown operation %d",op)));
        PG_RETURN_NULL();
    }

    switch(op) {

    case OP_REMOVE:

        if(apos < 0) {
            pfree(aset);
            PG_RETURN_ARRAYTYPE_P((construct_empty_array(INT4OID)));
        }

        bset = (Datum *)palloc(b_len * sizeof(Datum));
        it = GET_ARRAY_ITERATOR(b);
        bpos = -1;
        while(array_iterate(it,&v,&is_null)) {
            if(is_null) continue;
            if(set_contains_value(bset,bpos,v)) continue;
            bset[++bpos] = v;
        }
        array_free_iterator(it);

        if(bpos < 0) {
            //empty b_set. we can return aset immediately
            pfree(bset);
            r = construct_array(aset,apos+1,INT4OID,sizeof(int),true,'i');
            pfree(aset);
            PG_RETURN_ARRAYTYPE_P(r);
        }

        rset = (Datum *)palloc((apos+1) * sizeof(Datum));

        //iterate over aset and add to result_set only values not presented in bset
        rpos = -1;
        for(i = 0; i <= apos; i++) {
            if(set_contains_value(bset,bpos,aset[i]))
                continue;
            rset[++rpos] = aset[i];
        }

        pfree(aset);
        pfree(bset);

        if(rpos < 0) {
            pfree(rset);
            PG_RETURN_ARRAYTYPE_P((construct_empty_array(INT4OID)));
        }

        break; //case OP_REMOVE

    case OP_APPEND:

        //realloc aset to make space for possible values from array B
        aset = repalloc(aset,apos+1+b_len);
        it = GET_ARRAY_ITERATOR(b);
        while(array_iterate(it,&v,&is_null)) {
            if(is_null) continue;
            if(set_contains_value(aset,apos,v)) continue;
            aset[++apos] = v;
        }
        array_free_iterator(it);

        if(apos < 0 ) {
            pfree(aset);
            PG_RETURN_ARRAYTYPE_P((construct_empty_array(INT4OID)));
        }

        r = construct_array(aset,apos+1,INT4OID,sizeof(int),true,'i');
        pfree(aset);
        PG_RETURN_ARRAYTYPE_P(r);

        //no break because of return

    case OP_INTERSECTION:

        rset = (Datum *)palloc((apos+1) * sizeof(Datum));

        it = GET_ARRAY_ITERATOR(b);
        rpos = -1;
        while(array_iterate(it,&v,&is_null)) {
            if(is_null) continue;
            if(set_contains_value(rset,rpos,v)) continue;
            if(!set_contains_value(aset,apos,v)) continue;
            rset[++rpos] = v;
        }
        array_free_iterator(it);

        pfree(aset);

        if(rpos < 0) {
            pfree(rset);
            PG_RETURN_ARRAYTYPE_P((construct_empty_array(INT4OID)));
        }

        break; //case OP_INTERSECTION
    }

    r = construct_array(rset,rpos+1,INT4OID,sizeof(int),true,'i');
    pfree(rset);
    PG_RETURN_ARRAYTYPE_P(r);
}
