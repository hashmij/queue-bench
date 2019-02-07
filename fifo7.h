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

#define BUF_SIZE (4096-24)
struct cell_t {
    int32_t rank;
    int32_t gap;
    int32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* FFQ - IPDPS 17 */
struct queue_t {
	volatile int32_t head CACHE_ALIGNED;
	volatile int32_t tail CACHE_ALIGNED;
    struct cell_t data[QUEUE_SIZE] CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
    int i;
	memset(q, 0, sizeof(struct queue_t));
    q->head = q->tail = 0;

    for (i=0; i < QUEUE_SIZE; i++) {
        q->data[i].rank = -1;
        q->data[i].gap  = -1;
    }
}

void queue_fini(struct queue_t *q)
{
}

inline int available(struct queue_t *q) {
    uint32_t rank = q->head;
    struct cell_t *cell =  &q->data[rank & QUEUE_MASK];
    return (cell->rank == rank);
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    uint32_t tail = q->tail++;
    struct cell_t *cell = &q->data[tail & QUEUE_MASK];
    while (cell->rank != -1);
#if 0
    for (int d=60; d<len; d+=64) {
        __builtin_prefetch(cell->buf+d, 1, 1);
    }
    memcpy(cell->buf, c, len);
#endif
    cell->size = len;
    cell->rank = tail;
    return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    if (!available(q)) {
        return BUFFER_EMPTY;
    }
    uint32_t rank = q->head++;
    struct cell_t *cell =  &q->data[rank & QUEUE_MASK];
    while (cell->rank != rank);
    *len = cell->size;
    cell->rank = -1;
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
