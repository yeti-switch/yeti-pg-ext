#include "postgres.h"

#define _log(level,fmt, ...) elog(level,LOG_PREFIX fmt, ## __VA_ARGS__)

#define info(fmt, ...) _log(INFO, fmt, ## __VA_ARGS__)
#define dbg(fmt, ...) _log(DEBUG1, fmt, ## __VA_ARGS__)
#define warn(fmt, ...) _log(WARNING, fmt, ## __VA_ARGS__)
#define err(fmt, ...) _log(ERROR, fmt, ## __VA_ARGS__)

#define exit_err(fmt, ...) warn(fmt, ## __VA_ARGS__); PG_RETURN_NULL();
