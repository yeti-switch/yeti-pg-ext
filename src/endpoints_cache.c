#include "endpoints_cache.h"
#include "log.h"

#include "utils/hsearch.h"

#define LOG_PREFIX "endpoints_cache: "

#define HTAB_INIT_SIZE 256
#define EC_KEY_MAX_LENGTH 256
#define EC_RESPONSE_MAX_LENGTH 2 * 1024

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

int __ec_init(void);
int __ec_store(const char *key, const char *response, bool error);
int __ec_find(const char *key, char **response_out, bool *error_out);
int __ec_items_count(void);
int __ec_remove(const char *key);
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

int __ec_store(const char *key, const char *response, bool error) {
	EndpointCacheKey ckey;
	EndpointCacheEntry *entry;
	bool found;

	if (hash_tab == NULL) {
		__ec_init();
	}

	strncpy(ckey.v, key, EC_KEY_MAX_LENGTH);
	entry = hash_search(hash_tab, (void *)&ckey, HASH_ENTER, &found);
	Assert(_data == NULL);

	// store data
	strncpy(entry->response, response, EC_RESPONSE_MAX_LENGTH);
	entry->error = error;

	__ec_print_elements();
	return 0;
}

int __ec_find(const char *key, char **response_out, bool *error_out) {
	EndpointCacheKey ckey;
	EndpointCacheEntry *entry;
	bool found;

	if (hash_tab == NULL) {
		return -1;
	}

	strncpy(ckey.v, key, EC_KEY_MAX_LENGTH);
	entry = hash_search(hash_tab, (void *)&ckey, HASH_FIND, &found);

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

int __ec_remove(const char *key) {
	EndpointCacheKey ckey;
	EndpointCacheEntry *entry;
	bool found;

	if (hash_tab == NULL) {
		return -1;
	}

	strncpy(ckey.v, key, EC_KEY_MAX_LENGTH);
	entry = hash_search(hash_tab, (void *)&ckey, HASH_REMOVE, &found);

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
