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
#define QUEUE_SIZE (256) 
#define QUEUE_MASK (QUEUE_SIZE-1) 
#define CACHE_ALIGNED  __attribute__ ((aligned(64)))

#define BUF_SIZE (4096-4)
struct cell_t {
    volatile int32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* Better FF Queue */
struct queue_t {
	volatile uint32_t head CACHE_ALIGNED;
	volatile uint32_t tail CACHE_ALIGNED;
    struct cell_t data[QUEUE_SIZE] CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
	memset(q, 0, sizeof(struct queue_t));
    //q->data = aligned_alloc(PAGE_SIZE, QUEUE_SIZE * sizeof(struct cell_t));
    //memset(q->data, 0, QUEUE_SIZE * sizeof(struct cell_t));
}

void queue_fini(struct queue_t *q)
{
    //free(q->data);
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    uint32_t tail = __sync_fetch_and_add(&q->tail, 1) & QUEUE_MASK;
    struct cell_t *cell = &q->data[tail];
    while (cell->size);

    for (int d=60; d<len; d+=64) {
        __builtin_prefetch(cell->buf+d, 1, 1);
    }
    memcpy(cell->buf, c, len);
    cell->size = len;

	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    //uint32_t head = __sync_fetch_and_add(&q->head, 1) & QUEUE_MASK;
    uint32_t head = q->head++ & QUEUE_MASK;
    struct cell_t *cell = &q->data[head];
	while (!cell->size);

    *len = cell->size;
    memcpy(c, cell->buf, *len);
    cell->size = 0;

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
