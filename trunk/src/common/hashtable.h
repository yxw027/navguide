#ifndef _HASHTABLE_H
#define _HASHTABLE_H

#include <stdint.h>

typedef uint32_t hash_t;

typedef struct hashtable_entry hashtable_entry_t;
struct hashtable_entry
{
	const void *key, *value;
	hashtable_entry_t *next; // for internal use; don't use to iterate through.
};


typedef struct hashtable hashtable_t;
struct hashtable
{
	hashtable_entry_t **entries;
	int size;
	int capacity;
	hash_t (*hash)(const void*);
	int (*equals)(const void*, const void*);
};

typedef struct hashtable_iterator hashtable_iterator_t;
struct hashtable_iterator
{
	hashtable_t *ht;
	int  nextbucket;
	hashtable_entry_t *nextentry;
};

hashtable_t *hashtable_create(int capacity);

void hashtable_destroy(hashtable_t *ht);
void hashtable_put(hashtable_t *ht, const void *key, const void *value);
const void* hashtable_get(hashtable_t *ht, const void *key);
void hashtable_remove(hashtable_t *ht, const void *key);
int hashtable_size(hashtable_t *ht);

hashtable_iterator_t* hashtable_iterator_create(hashtable_t *ht);
void hashtable_iterator_destroy(hashtable_iterator_t *hi);
hashtable_entry_t* hashtable_iterator_next(hashtable_iterator_t *hi);

#endif
