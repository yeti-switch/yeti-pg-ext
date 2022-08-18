#include "postgres.h"
#include <fmgr.h>

text *replace(text *in, bool *replaced);
Datum get_in_copy(PG_FUNCTION_ARGS);
Datum get_in_copy_array(PG_FUNCTION_ARGS);
