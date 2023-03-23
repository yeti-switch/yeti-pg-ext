#pragma once

#include "postgres.h"
#include "fmgr.h"

#define CPY_TEXTARG_TO_STR(index, string) {\
	int l;\
	text *text_p;\
	text_p = PG_GETARG_TEXT_P(index);\
	l = VARSIZE_ANY_EXHDR(text_p);\
	string = palloc(l + 1);\
	memcpy(string, VARDATA_ANY(text_p), l);\
	string[l] = '\0';\
}

#define STR_FROM_TEXTARG(index) ({\
	char *result;\
	CPY_TEXTARG_TO_STR(index, result);\
	result;\
})
