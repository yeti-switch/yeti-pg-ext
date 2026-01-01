#include "exported_functions.h"
#include "log.h"

#include <utils/builtins.h>
#include <utils/array.h>
#include <utils/jsonb.h>
#include <catalog/pg_type.h>

#include <string.h>

#define LOG_PREFIX "process_templates: "
#define INITIAL_BUF_SIZE 64

Datum replace_placeholders(Datum tpl, Jsonb *values);

PG_FUNCTION_INFO_V1(process_templates);
Datum process_templates(PG_FUNCTION_ARGS)
{
    enum args_idx {
        ARG_TEMPLATES = 0,
        ARG_VARS,
    };

    Datum v;
    bool is_null;
    ArrayIterator it;
    ArrayBuildState *array_state;
    Jsonb *values;
    bool is_array = false;

    if(PG_ARGISNULL(ARG_TEMPLATES))
        PG_RETURN_NULL();

    switch(get_fn_expr_argtype(fcinfo->flinfo, ARG_TEMPLATES)) {
    case TEXTARRAYOID:
        is_array = true;
        break;
    case TEXTOID:
        break;
    default:
        //unexpected ARG_TEMPLATES type
        PG_RETURN_NULL();
    }

    if(PG_ARGISNULL(ARG_VARS)) {
        if(is_array) {
            PG_RETURN_ARRAYTYPE_P(PG_GETARG_ARRAYTYPE_P_COPY(ARG_TEMPLATES));
        } else {
            PG_RETURN_NULL();
        }
    }

    values = PG_GETARG_JSONB_P(ARG_VARS);
    if(!JsonContainerIsObject(&values->root)) {
        if(is_array) {
            PG_RETURN_ARRAYTYPE_P(PG_GETARG_ARRAYTYPE_P_COPY(ARG_TEMPLATES));
        } else {
            PG_RETURN_NULL();
        }
    }

    if(!is_array) {
        return replace_placeholders(PG_GETARG_DATUM(ARG_TEMPLATES), values);
    }

    array_state = initArrayResult(TEXTOID, CurrentMemoryContext, false);
    it = array_create_iterator(PG_GETARG_ARRAYTYPE_P(ARG_TEMPLATES), 0, NULL);
    while(array_iterate(it,&v,&is_null)) {
        if(is_null) {
            array_state = accumArrayResult(
                array_state,
                PointerGetDatum(0), true, TEXTOID,
                CurrentMemoryContext);
                continue;
        }

        array_state = accumArrayResult(
            array_state,
            replace_placeholders(v, values), false, TEXTOID,
            CurrentMemoryContext);
    }

    return makeArrayResult(array_state, CurrentMemoryContext);
}

Datum replace_placeholders(Datum tpl, Jsonb *values)
{
    static char placeholder_start[2] = {'{', '{'};
    static char placeholder_end[2] = {'}', '}'};
    static char prefix[5] = {'v','a','r','s','.'};

    char *tpl_ptr = VARDATA_ANY(tpl);
    char *tpl_end = tpl_ptr + VARSIZE_ANY_EXHDR(tpl);
    char *tmp, *serialized_num;

    JsonbContainer *j = &values->root;
    JsonbValue v, *v_ptr;

    StringInfoData sinfo;

    enum parser_state {
        ST_NORMAL,
        ST_PLACEHOLDER,
    } st = ST_NORMAL;

#if PGVER >= 1800
    initStringInfoExt(&sinfo, INITIAL_BUF_SIZE);
#else
    initStringInfo(&sinfo);
#endif

    while(tpl_ptr < tpl_end) {
        switch(st) {
        case ST_NORMAL:
            tmp = memmem(
                tpl_ptr, tpl_end - tpl_ptr,
                placeholder_start, sizeof(placeholder_start));
            if(NULL==tmp) {
                //no more placeholders. copy remained tail [tpl_ptr, end]
                appendBinaryStringInfoNT(&sinfo, tpl_ptr, tpl_end - tpl_ptr);
                tpl_ptr = tpl_end;
                break;
            }

            //found placeholder. copy leading head [tpl_ptr,tmp]
            appendBinaryStringInfoNT(&sinfo, tpl_ptr, tmp - tpl_ptr);

            tpl_ptr=tmp+sizeof(placeholder_start);
            st = ST_PLACEHOLDER;

            break;

        case ST_PLACEHOLDER:
            tmp = memmem(
                tpl_ptr, tpl_end - tpl_ptr,
                placeholder_end, sizeof(placeholder_end));

            if(NULL==tmp) {
                //no placeholder end. copy remained tail [tpl_ptr, end]
                appendBinaryStringInfoNT(&sinfo, tpl_ptr, tpl_end - tpl_ptr);
                tpl_ptr = tpl_end;
                break;
            }

            //found placeholder end. placeholder name is: [tpl_ptr, tmp]

            //prepare value name (check for "var." prefix and remove it)
            if(((tmp-tpl_ptr) <= sizeof(prefix))
               || (0!=memcmp(tpl_ptr,prefix,sizeof(prefix))))
            {
                tpl_ptr = tmp + sizeof(placeholder_end);
                st = ST_NORMAL;
                break;
            }
            tpl_ptr += sizeof(prefix);

            //lookup for value
            v_ptr = getKeyJsonValueFromContainer(
                j,
                tpl_ptr, tmp-tpl_ptr,
                &v
            );

            tpl_ptr = tmp + sizeof(placeholder_end);
            st = ST_NORMAL;

            if(NULL==v_ptr) {
                //append with 'null' fop nx variables
                appendStringInfoString(&sinfo, "null");
                break;
            }

            //serialize value and append to the buf
            //see: src/backend/utils/adt/jsonb.c:JsonbUnquote()
            switch(v.type) {
            case jbvNull:
                appendStringInfoString(&sinfo, "null");
                break;
            case jbvString:
                appendBinaryStringInfoNT(&sinfo,
                    v.val.string.val, v.val.string.len);
                break;
            case jbvNumeric:
                serialized_num = DatumGetCString(DirectFunctionCall1(
                    numeric_out, PointerGetDatum(v.val.numeric)));
                appendStringInfoString(&sinfo, serialized_num);
                pfree(serialized_num);
                break;
            case jbvBool:
                appendStringInfoString(&sinfo, v.val.boolean ? "true" : "false");
                break;
            default:
                //ignore non-scalar types
                break;
            }
            break;
        } //switch(st)
    }

    return PointerGetDatum(
        cstring_to_text_with_len(
            sinfo.data, sinfo.len));
}
