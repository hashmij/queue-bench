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

#define PAGE_SIZE 4096
#define BUF_SIZE (4096)
#define QUEUE_SIZE (256) 
#define QUEUE_MASK (QUEUE_SIZE-1) 
#define CACHE_ALIGNED __attribute__ ((aligned(64)))

/* Non-intrusive FF Queue */
struct queue_t {
	volatile uint32_t head CACHE_ALIGNED;
	volatile uint32_t tail CACHE_ALIGNED;
    uint32_t lens[QUEUE_SIZE] CACHE_ALIGNED;
    char *data CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
	memset(q, 0, sizeof(struct queue_t));
    //q->lens = aligned_alloc(PAGE_SIZE, QUEUE_SIZE * sizeof(uint32_t));
    //memset(q->lens, 0, QUEUE_SIZE * sizeof(uint32_t));
    q->data = aligned_alloc(PAGE_SIZE, QUEUE_SIZE * BUF_SIZE * sizeof(char));
    memset(q->data, 0, QUEUE_SIZE * BUF_SIZE * sizeof(char));
}

void queue_fini(struct queue_t *q)
{
    //free(q->lens);
    free(q->data);
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    uint32_t tail = q->tail;
	if ( q->lens[tail] )
		return BUFFER_FULL;

    char *buf = __builtin_assume_aligned(q->data + tail * BUF_SIZE, 64);
    __builtin_prefetch(buf, 1, 1);
    memcpy(buf, c, len);
	q->lens[tail] = len;

    q->tail = (tail + 1) & QUEUE_MASK;
	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    uint32_t head = q->head;
	if ( !q->lens[head] )
		return BUFFER_EMPTY;

    char *buf = __builtin_assume_aligned(q->data + head * BUF_SIZE, 64);
    __builtin_prefetch(buf, 0, 1);
	*len = q->lens[head];
    memcpy(c, buf, *len);
	q->lens[head] = 0;

    q->head = (head + 1) & QUEUE_MASK;
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
