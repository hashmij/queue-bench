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
#define QUEUE_SIZE (256) 
#define CACHE_ALIGNED  __attribute__ ((aligned(128)))

#define BUF_SIZE (4096-8)
struct cell_t {
    uint32_t flag;
    uint32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* Basic FF Queue */
struct queue_t {
	/* Mostly accessed by producer. */
	volatile	uint32_t	head CACHE_ALIGNED;

	/* Mostly accessed by consumer. */
	volatile 	uint32_t	tail CACHE_ALIGNED;

	/* accessed by both producer and comsumer */
    struct cell_t *data CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
	memset(q, 0, sizeof(struct queue_t));
    q->data = aligned_alloc(PAGE_SIZE, QUEUE_SIZE * sizeof(struct cell_t));
    memset(q->data, 0, QUEUE_SIZE * sizeof(struct cell_t));
}

void queue_fini(struct queue_t *q)
{
    free(q->data);
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    struct cell_t *tail = &q->data[q->tail];
    if (tail->flag)
		return BUFFER_FULL;

    tail->size = len;
    memcpy(tail->buf, c, len);
    tail->flag = CELL_FULL;

	q->tail ++;
	if ( q->tail >= QUEUE_SIZE ) {
		q->tail = 0;
	}

	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    struct cell_t *head = &q->data[q->head];
	if (!head->flag)
		return BUFFER_EMPTY;

    *len = head->size;
    memcpy(c, head->buf, *len);
    head->flag = CELL_NULL;

	q->head ++;
	if ( q->head >= QUEUE_SIZE )
		q->head = 0;

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
