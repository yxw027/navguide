#ifndef _DGC_VECTOR_H
#define _DGC_VECTOR_H

typedef struct dgc_vector dgc_vector_t;

struct dgc_vector
{
	void **items;
	int size;
	int capacity;
};

typedef struct dgc_vector_iterator dgc_vector_iterator_t;

struct dgc_vector_iterator
{
	dgc_vector_t *v;
	int nextidx;
};

dgc_vector_t *dgc_vector_create(int capacity);
void dgc_vector_destroy(dgc_vector_t *v);
void dgc_vector_ensure_capacity(dgc_vector_t *v, int newcapacity);
void dgc_vector_add(dgc_vector_t *v, void *item);
void dgc_vector_put(dgc_vector_t *v, int idx, void *item);
void *dgc_vector_get(dgc_vector_t *v, int idx);
int dgc_vector_size(dgc_vector_t *v);
void dgc_vector_sort(dgc_vector_t *v, int (*compar)(const void *, const void *));
void dgc_vector_delete(dgc_vector_t *v, int idx);
void dgc_vector_delete_shuffle(dgc_vector_t *v, int idx);
void dgc_vector_delete_find(dgc_vector_t *v, void *n);
int dgc_vector_is_member(dgc_vector_t *v, void *item);

dgc_vector_iterator_t *dgc_vector_iterator_create(dgc_vector_t *v);
void dgc_vector_iterator_destroy(dgc_vector_iterator_t *v);
void *dgc_vector_iterator_next(dgc_vector_iterator_t *v);

#endif
