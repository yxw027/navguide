#include <stdlib.h>
#include <assert.h>

#include "dgc_vector.h"

dgc_vector_t *dgc_vector_create(int capacity)
{
	dgc_vector_t *v = (dgc_vector_t*) calloc(1, sizeof(dgc_vector_t));
	v->items = (void**) calloc(capacity, sizeof(void*));
	v->capacity = capacity;

	return v;
}

void dgc_vector_destroy(dgc_vector_t *v)
{
	free(v->items);
	free(v);
}

void dgc_vector_grow(dgc_vector_t *v)
{
	if (v->size < v->capacity)
		return;

	dgc_vector_ensure_capacity(v, v->size * 2);
}

void dgc_vector_ensure_capacity(dgc_vector_t *v, int newcapacity)
{
	v->items = (void**) realloc(v->items, newcapacity * sizeof(void*));
	v->capacity = newcapacity;
}

void dgc_vector_add(dgc_vector_t *v, void *item)
{
	dgc_vector_grow(v);

	v->items[v->size++] = item;
}

void dgc_vector_put(dgc_vector_t *v, int idx, void *item)
{
	assert(idx < v->size);

	v->items[idx] = item;
}

void *dgc_vector_get(dgc_vector_t *v, int idx)
{
	assert(idx < v->size);
	
	return v->items[idx];
}

int dgc_vector_size(dgc_vector_t *v)
{
	return v->size;
}

void dgc_vector_sort(dgc_vector_t *v, int (*compar)(const void *, const void *))
{
	qsort(v->items, v->size, sizeof(void*), compar);
}

void dgc_vector_delete_find(dgc_vector_t *v, void *n)
{
	int i;
	for (i = 0; i < v->size; i++) {
		if (v->items[i]==n) {
			dgc_vector_delete(v, i);
			i--;
		}
	}
}

void dgc_vector_delete(dgc_vector_t *v, int idx)
{
	int i;
	assert(idx < v->size);

	for (i = idx+1; i < v->size; i++)
		v->items[i-1] = v->items[i];

	v->size--;
}

void dgc_vector_delete_shuffle(dgc_vector_t *v, int idx)
{
	assert(idx < v->size);

	v->items[idx] = v->items[v->size-1];

	v->size--;
}

int dgc_vector_is_member(dgc_vector_t *v, void *item)
{
	int i;

	for (i = 0; i < v->size; i++)
		if (v->items[i] == item)
			return 1;

	return 0;
}

dgc_vector_iterator_t *dgc_vector_iterator_create(dgc_vector_t *v)
{
	dgc_vector_iterator_t *vi = (dgc_vector_iterator_t*) calloc(1, sizeof(dgc_vector_iterator_t));
	vi->v = v;
	vi->nextidx = 0;

	return vi;
}

void dgc_vector_iterator_destroy(dgc_vector_iterator_t *vi)
{
	free(vi);
}

void *dgc_vector_iterator_next(dgc_vector_iterator_t *vi)
{
	if (vi->nextidx >= vi->v->size)
		return NULL;

	return vi->v->items[vi->nextidx++];
}
