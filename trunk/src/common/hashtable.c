#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#include "hashtable.h"

#define MINIMUM_CAPACITY 16

#define HASH_STRING_RIGHT_SHIFT ((sizeof(hash_t)*8)-9)

hash_t hash_string(const void *t)
{
	const char *s = (char*) t;
	hash_t h = 0;
	int idx = 0;

	while (s[idx] !=0 ) {
		h=(h<<7) + s[idx] + (h>>HASH_STRING_RIGHT_SHIFT);
		idx++;
	}
	
	return h;
}

int equals_string(const void *a, const void *b)
{
	return !strcmp((char*) a, (char*) b);
}


static int max(int a, int b)
{
	return a > b ? a : b;
}

hashtable_t *hashtable_create(int capacity)
{
	hashtable_t *ht = (hashtable_t*) calloc(1, sizeof(hashtable_t));
	ht->size = 0;
	ht->capacity = max(capacity, MINIMUM_CAPACITY);
	ht->entries = (hashtable_entry_t**) calloc(ht->capacity, sizeof(hashtable_entry_t*));
	ht->hash = hash_string;
	ht->equals = equals_string;
	return ht;
}

void hashtable_destroy(hashtable_t *ht)
{
	int i;

	for (i = 0; i < ht->capacity; i++) {
		hashtable_entry_t *e = ht->entries[i];
		while (e != NULL) {
			hashtable_entry_t *n = e->next;
			free(e);
			e = n;
		}
	}
		
	free(ht->entries);
	free(ht);
}

void hashtable_grow(hashtable_t *ht)
{
	// no need to grow?
	if ((ht->size+1) < (ht->capacity*2/3))
		return;

	int newcapacity = ht->size*3/2;
	if (newcapacity < MINIMUM_CAPACITY)
		newcapacity = MINIMUM_CAPACITY;

	hashtable_entry_t** newentries = (hashtable_entry_t**) calloc(newcapacity, sizeof(hashtable_entry_t));

	hashtable_iterator_t *hi = hashtable_iterator_create(ht);
	while (hashtable_iterator_next(hi)) {

		hashtable_entry_t *e = hashtable_iterator_next(hi);

		hash_t h = ht->hash(e->key);
		int idx = h % ht->capacity;
		
		e->next = newentries[idx];
		newentries[idx] = e->next;
	}

	hashtable_iterator_destroy(hi);
	ht->entries = newentries;
	ht->capacity = newcapacity;
	free(ht->entries);
}

void hashtable_put(hashtable_t *ht, const void *key, const void *value)
{
	hash_t h = ht->hash(key);

	int idx = h % ht->capacity;
	hashtable_entry_t *e = ht->entries[idx];
	while (e != NULL) {
		// replace?
		if (ht->equals(e->key, key)) {
			e->value = value;
			return;
		}
		
		e = e->next;
	}

	// conditionally grow
	hashtable_grow(ht);

	e = (hashtable_entry_t*) calloc(1, sizeof(hashtable_entry_t));
	e->key = key;
	e->value = value;
	e->next = ht->entries[idx];
	
	ht->entries[idx] = e;
	ht->size++;
}

const void* hashtable_get(hashtable_t *ht, const void *key)
{
	hash_t h = ht->hash(key);

	int idx = h % ht->capacity;
	hashtable_entry_t *e = ht->entries[idx];
	while (e != NULL) {
		if (ht->equals(e->key, key)) {
			return e->value;
		}
		
		e = e->next;
	}

	return NULL;
}

void hashtable_remove(hashtable_t *ht, const void *key)
{
	hash_t h = ht->hash(key);

	int idx = h % ht->capacity;
	hashtable_entry_t *e = ht->entries[idx];
	hashtable_entry_t **pe = &ht->entries[idx];
	while (e != NULL) {
		// replace?
		if (ht->equals(e->key, key)) {
			*pe = e->next;
			free(e);
			ht->size--;
			return;
		}
		
		pe = &(e->next);
		e = e->next;
	}
}

hashtable_iterator_t* hashtable_iterator_create(hashtable_t *ht)
{
	hashtable_iterator_t *hi = (hashtable_iterator_t*) calloc(1, sizeof(hashtable_iterator_t));
	hi->ht = ht;
	hi->nextbucket = 0;
	hi->nextentry = NULL;

	return hi;
}

void hashtable_iterator_destroy(hashtable_iterator_t *hi)
{
	free(hi);
}

hashtable_entry_t* hashtable_iterator_next(hashtable_iterator_t *hi)
{
	while (hi->nextentry == NULL && hi->nextbucket < hi->ht->capacity)
		hi->nextentry = hi->ht->entries[hi->nextbucket++];

	if (hi->nextentry == NULL)
		return NULL;

	hashtable_entry_t *e = hi->nextentry;
	hi->nextentry = hi->nextentry->next;

	return e;
}

int hashtable_size(hashtable_t *ht)
{
	return ht->size;
}

