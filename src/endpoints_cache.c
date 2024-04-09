#include "endpoints_cache.h"
#include "log.h"

#if PGVER >= 1600
#include "varatt.h"
#endif

#include "utils/hsearch.h"

#include <math.h>

#define LOG_PREFIX "endpoints_cache: "

#define HTAB_INIT_SIZE 256
#define EC_KEY_MAX_LENGTH 256
#define EC_RESPONSE_MAX_LENGTH 2 * 1024


static char key_buf[EC_KEY_MAX_LENGTH];

/*
 * hash table key
 */

typedef struct {
	char v[EC_KEY_MAX_LENGTH];
} EndpointCacheKey;

/*
 * hash table element
 */

typedef struct {
	EndpointCacheKey key;
	char response[EC_RESPONSE_MAX_LENGTH];
	bool error;
} EndpointCacheEntry;

/*
 * local vars
 */

HTAB *hash_tab;

/*
 * func prototypes
 */

void set_key_buf(const text *key);

int __ec_init(void);
int __ec_store(const text *key,const text *response, bool error);
int __ec_find(const text *, char **response_out, bool *error_out);
int __ec_items_count(void);
int __ec_remove(const text *);
int __ec_remove_all(void);
int __ec_print_elements(void);
int __ec_destroy(void);

/*
 * 'endpoints_cache' struct
 */

const struct endpoints_cache EndpointsCache = {
	.store = __ec_store,
	.find = __ec_find,
	.items_count = __ec_items_count,
	.remove = __ec_remove,
	.remove_all = __ec_remove_all,
	.destroy = __ec_destroy
};

/*
 * func implementations
 */

void set_key_buf(const text *key)
{
	const int size = Min(VARSIZE_ANY_EXHDR(key), EC_KEY_MAX_LENGTH);
	bzero(key_buf, EC_KEY_MAX_LENGTH);
	strncpy(key_buf, VARDATA_ANY(key), size);
}

int __ec_init(void) {
	HASHCTL info;

	if (hash_tab != NULL) {
		return 1;
	}

	info.keysize = sizeof(EndpointCacheKey);
	info.entrysize = sizeof(EndpointCacheEntry);
	hash_tab = hash_create("endpoints_cache",
							HTAB_INIT_SIZE, &info, HASH_ELEM | HASH_BLOBS);
	return 0;
}

int __ec_store(const text *key, const text *response, bool error) {
	EndpointCacheEntry *entry;
	bool found;

	if (hash_tab == NULL) {
		__ec_init();
	}

	set_key_buf(key);
	entry = hash_search(hash_tab, (const void *)key_buf, HASH_ENTER, &found);
	Assert(entry);

	// store data
	bzero(entry->response, EC_RESPONSE_MAX_LENGTH);
	strncpy(entry->response,
		VARDATA_ANY(response),
		Min(VARSIZE_ANY_EXHDR(response), EC_RESPONSE_MAX_LENGTH));
	entry->error = error;

	__ec_print_elements();
	return 0;
}

int __ec_find(const text *key, char **response_out, bool *error_out) {
	EndpointCacheEntry *entry;
	bool found;

	if (hash_tab == NULL) {
		return -1;
	}

	set_key_buf(key);
	entry = hash_search(hash_tab, (const void *)key_buf, HASH_FIND, &found);

	if (entry == NULL) {
		return -1;
	}

	*response_out = entry->response;
	*error_out = entry->error;
	return 0;
}

int __ec_items_count(void) {
	if (hash_tab == NULL) {
		return 0;
	}

	return hash_get_num_entries(hash_tab);
}

int __ec_remove(const text *key) {
	EndpointCacheEntry *entry;
	bool found;

	if (hash_tab == NULL) {
		return -1;
	}

	set_key_buf(key);
	entry = hash_search(hash_tab, (const void *)key_buf, HASH_REMOVE, &found);

	if (entry == NULL) {
		return -1;
	}

	__ec_print_elements();
	return 0;
}

int __ec_remove_all(void) {
	HASH_SEQ_STATUS status;
	EndpointCacheEntry *entry;

	if (hash_tab == NULL) {
		return 1;
	}

	hash_seq_init(&status, hash_tab);
	while ((entry = (EndpointCacheEntry *)hash_seq_search(&status)) != NULL) {
		if (hash_search(hash_tab, (void *)&entry->key, HASH_REMOVE, NULL) == NULL) {
			return 1;
		}
	}

	__ec_print_elements();
	return 0;
}

int __ec_print_elements(void) {
	HASH_SEQ_STATUS status;
	EndpointCacheEntry *entry;

	if (hash_tab == NULL) {
		return 1;
	}

	info("number of elements: %d", __ec_items_count());

	hash_seq_init(&status, hash_tab);
	while ((entry = (EndpointCacheEntry *)hash_seq_search(&status)) != NULL) {
		info("key = %s, response = %s, error = %d",
			  entry->key.v, entry->response, entry->error);
	}

	return 0;
}

int __ec_destroy(void) {
	if (hash_tab == NULL) {
		return 1;
	}

	__ec_remove_all();
	hash_destroy(hash_tab);
	hash_tab = NULL;
	return 0;
}
