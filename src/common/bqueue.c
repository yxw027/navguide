#include <pthread.h>
#include <stdlib.h>
#include <errno.h>
#include <assert.h>

#include "bqueue.h"

//#include "list.h"
#ifndef __LIST_H__
#define __LIST_H__

#include <stddef.h>

/*
** Generic circular doubly linked list implementation.
**
** list_t is the head of the list.
** list_link_t should be included in structures which want to be
**     linked on a list_t.
**
** All of the list functions take pointers to list_t and list_link_t
** types, unless otherwise specified.
**
** list_init(list) initializes a list_t to an empty list.
**
** list_empty(list) returns 1 iff the list is empty.
**
** Insertion functions.
**   list_insert_head(list, link) inserts at the front of the list.
**   list_insert_tail(list, link) inserts at the end of the list.
**   list_insert_before(olink, nlink) inserts nlink before olink in list.
**
** Removal functions.
** Head is list->l_next.  Tail is list->l_prev.
** The following functions should only be called on non-empty lists.
**   list_remove(link) removes a specific element from the list.
**   list_remove_head(list) removes the first element.
**   list_remove_tail(list) removes the last element.
**
** Item accessors.
**   list_item(link, type, member) given a list_link_t* and the name
**      of the type of structure which contains the list_link_t and
**      the name of the member corresponding to the list_link_t,
**      returns a pointer (of type "type*") to the item.
**
** To iterate over a list,
**
**    list_link_t *link;
**    for (link = list->l_next;
**         link != list; link = link->l_next)
**       ...
** 
** Or, use the macros, which will work even if you list_remove() the
** current link:
** 
**    type iterator;
**    list_iterate_begin(list, iterator, type, member) {
**        ... use iterator ...
**    } list_iterate_end;
*/

typedef struct llist {
	struct llist *l_next;
	struct llist *l_prev;
} list_t, list_link_t;

#define list_init(list) \
    (list)->l_next = (list)->l_prev = (list);

#define list_link_init(link) \
    (link)->l_next = (link)->l_prev = NULL;

#define list_empty(list) \
	((list)->l_next == (list))

#define list_insert_before(old, new) \
	do { \
		list_link_t *prev = (new); \
		list_link_t *next = (old); \
		prev->l_next = next; \
		prev->l_prev = next->l_prev; \
		next->l_prev->l_next = prev; \
		next->l_prev = prev; \
	} while(0)

#define list_insert_head(list, link) \
	list_insert_before((list)->l_next, link)

#define list_insert_tail(list, link) \
	list_insert_before(list, link)

#define list_remove(link) \
	do { \
		list_link_t *ll = (link); \
		list_link_t *prev = ll->l_prev; \
		list_link_t *next = ll->l_next; \
		prev->l_next = next; \
		next->l_prev = prev; \
		ll->l_next = ll->l_prev = 0; \
	} while(0)

#define list_remove_head(list) \
	list_remove((list)->l_next)

#define list_remove_tail(list) \
	list_remove((list)->l_prev)

#define list_item(link, type, member) \
	(type*)((char*)(link) - offsetof(type, member))

#define list_head(list, type, member) \
	list_item((list)->l_next, type, member)

#define list_tail(list, type, member) \
	list_item((list)->l_prev, type, member)

#define list_iterate_begin(list, var, type, member) \
	do { \
		list_link_t *__link; \
		list_link_t *__next; \
		for (__link = (list)->l_next; \
		     __link != (list); \
		     __link = __next) { \
			var = list_item(__link, type, member); \
			__next = __link->l_next;

#define list_iterate_end() \
		    } \
    	} while(0)

#define list_iterate_reverse_begin(list, var, type, member) \
    do { \
        list_link_t *__link; \
        list_link_t *__prev; \
        for (__link = (list)->l_prev; \
             __link != (list); \
             __link = __prev) { \
            var = list_item(__link, type, member); \
            __prev = __link->l_prev;

#define list_iterate_reverse_end() \
        } \
    } while(0)

#endif /* __LIST_H__ */

struct _bqueue {
    pthread_mutex_t q_mtx;              /* for synchronization */
    pthread_cond_t q_cond;              /* for a-wakin' up */
    list_t q_list;                      /* stores the data */
    int q_status;                       /* 0 if init'd, 1 if destroyed */
    int q_count;                        /* count of waiting threads */
};

typedef struct bqueue_item {
    void *qi_data;                      /* holds the data */
    list_link_t qi_link;                /* to be able to place it in a list */
} bqueue_item_t;

static int __bqueue_dequeue( bqueue_t *q, void **data );

/* initialize a blocking queue */
bqueue_t* bqueue_create() {
    bqueue_t *q = (bqueue_t*)malloc(sizeof(bqueue_t));

    pthread_mutex_init( &q->q_mtx, NULL );
    pthread_cond_init( &q->q_cond, NULL );

    list_init( &q->q_list );
    q->q_status = q->q_count = 0;

    return q;
}

/* destroy a blocking queue */
int bqueue_destroy( bqueue_t *q ) {
    bqueue_item_t *qi;

    pthread_mutex_lock( &q->q_mtx );

    assert( !q->q_status );

    q->q_status = 1;
    pthread_cond_broadcast( &q->q_cond );

    /* wait for all bqueue_debqueue-ing threads to wake and notice that
       the bqueue is going away */
    while( q->q_count )
        pthread_cond_wait( &q->q_cond, &q->q_mtx );

    /* free all list elements, leaving data */
    list_iterate_begin( &q->q_list, qi, bqueue_item_t, qi_link ) {
        free( qi );
    } list_iterate_end();

    free( q );

    return 0;
}

/* enqueue an item into the queue. if the queue has been
 * destroyed, returns -EINVAL */
int bqueue_enqueue( bqueue_t *q, void *data ) {
    int ret;
    bqueue_item_t *qi;
   
    assert( data );
  
    pthread_mutex_lock( &q->q_mtx );

    if( q->q_status ) {
        pthread_mutex_unlock( &q->q_mtx );

        ret = -EINVAL;
    }
    else {
        qi = (bqueue_item_t*) malloc( sizeof( bqueue_item_t ) );
        qi->qi_data = data;
        list_link_init( &qi->qi_link );

        list_insert_tail( &q->q_list, &qi->qi_link );

        pthread_cond_signal( &q->q_cond );
        pthread_mutex_unlock( &q->q_mtx );

        ret = 0;
    }

    return ret;
}

/* dequeues an item from the queue... if the queue has been destroyed or
 * is destroyed while we're blocked, returns -EINVAL */
int bqueue_dequeue( bqueue_t *q, void **data ) {
    int ret;

    pthread_mutex_lock( &q->q_mtx );
    ret = __bqueue_dequeue( q, data );
    pthread_mutex_unlock( &q->q_mtx );

    return ret;
}

/* returns 1 if the queue is empty, 0 otherwise */
int bqueue_empty( bqueue_t *q ) {
    int ret;

    pthread_mutex_lock( &q->q_mtx );
    ret = list_empty( &q->q_list );
    pthread_mutex_unlock( &q->q_mtx );

    return ret;
}

/* attempts to dequeue an item from the queue... if there are no items
 * in the queue, rather than blocking we simply return 1 and *data has
 * an undefined value */
int bqueue_trydequeue( bqueue_t *q, void **data ) {
    int ret;

    pthread_mutex_lock( &q->q_mtx );

    if( !list_empty( &q->q_list ) )
        ret = __bqueue_dequeue( q, data );
    else
        ret = 1;

    pthread_mutex_unlock( &q->q_mtx );

    return ret;
}

/* dequeues an item from the queue... if the queue has been destroyed or
 * is destroyed while we're blocked, returns -EINVAL */
int __bqueue_dequeue( bqueue_t *q, void **data ) {
    int ret;
    bqueue_item_t *qi;

    q->q_count++;

    while( list_empty( &q->q_list ) && !q->q_status )
        pthread_cond_wait( &q->q_cond, &q->q_mtx );

    q->q_count--;

    /* the queue is being destroyed */
    if( q->q_status ) {
        if( q->q_count == 0 )
            pthread_cond_signal( &q->q_cond );

        ret = -EINVAL;
    }
    else {
        qi = list_head( &q->q_list, bqueue_item_t, qi_link );
        list_remove_head( &q->q_list );
        *data = qi->qi_data;
        free( qi );

        ret = 0;
    }

    return ret;
}
