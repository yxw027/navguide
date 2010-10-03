// file: bqueue.h
// desc: implementation of a blocking queue

#ifndef __BQUEUE_H__
#define __BQUEUE_H__

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _bqueue bqueue_t;

bqueue_t* bqueue_create();
int bqueue_destroy( bqueue_t *q );

/* enqueues a data pointer.  it is not valid to enqueue NULL
 */
int bqueue_enqueue( bqueue_t *q, void *data );

int bqueue_dequeue( bqueue_t *q, void **data );

/* returns non-zero if the queue is empty, 0 if the queue is not empty
 */
int bqueue_empty( bqueue_t *q );

/* attempts to dequeue an item from the queue... if there are no items
 * in the queue, rather than blocking we simply return 1 and *data has
 * an undefined value 
 *
 * on sucess, returns 0
 * */
int bqueue_trydequeue( bqueue_t *q, void **data );

#ifdef __cplusplus
}
#endif

#endif /* __BQUEUE_H__ */
