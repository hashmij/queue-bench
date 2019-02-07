#ifndef _FIFO_H_
#define _FIFO_H_

#include <unistd.h>
#include <stdio.h>
#include <pthread.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>

#define SUCCESS 0
#define BUFFER_FULL -1
#define BUFFER_EMPTY -2

#define CELL_FULL 1
#define CELL_NULL 0

#define PAGE_SIZE 4096
#define BUF_SIZE (4096-8)
#define QUEUE_SIZE (256) 
#define QUEUE_MASK (QUEUE_SIZE-1) 
#define CACHE_ALIGNED  __attribute__ ((aligned(64)))

/* Dummy FF Queue */
struct queue_t {
	uint32_t head CACHE_ALIGNED;
	uint32_t tail CACHE_ALIGNED;
    uint32_t *volatile data CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
	memset(q, 0, sizeof(struct queue_t));
    q->data = aligned_alloc(PAGE_SIZE, QUEUE_SIZE * sizeof(uint32_t));
    memset(q->data, 0, QUEUE_SIZE * sizeof(uint32_t));
}

void queue_fini(struct queue_t *q)
{
    free(q->data);
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
	if ( q->data[q->tail] )
		return BUFFER_FULL;

	q->data[q->tail] = CELL_FULL;

    q->tail = (q->tail + 1) & QUEUE_MASK;
	/*q->tail ++;
	if ( q->tail >= QUEUE_SIZE ) {
		q->tail = 0;
	}*/

	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
	if ( !q->data[q->head] )
		return BUFFER_EMPTY;

	q->data[q->head] = CELL_NULL;

    q->head = (q->head + 1) & QUEUE_MASK;
	/*q->head ++;
	if ( q->head >= QUEUE_SIZE ) {
		q->head = 0;
    }*/

	return SUCCESS;
}

inline uint64_t read_tsc()
{
        uint64_t        time;
        uint32_t        msw   , lsw;
        __asm__         __volatile__("rdtsc\n\t"
                        "movl %%edx, %0\n\t"
                        "movl %%eax, %1\n\t"
                        :         "=r"         (msw), "=r"(lsw)
                        :   
                        :         "%edx"      , "%eax");
        time = ((uint64_t) msw << 32) | lsw;
        return time;
}

#endif
