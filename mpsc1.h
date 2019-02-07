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

#define BUF_SIZE (4096-8)
struct cell_t {
    volatile uint32_t seq;
    volatile int32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* Better FF Queue */
struct queue_t {
	uint32_t head CACHE_ALIGNED;
	uint32_t tail CACHE_ALIGNED;
    struct cell_t data[QUEUE_SIZE] CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
	memset(q, 0, sizeof(struct queue_t));
    for (int i=0; i<QUEUE_SIZE; i++) {
        q->data[i].seq = i;
    }
}

void queue_fini(struct queue_t *q)
{
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    uint32_t head = q->head;
    for (;;) {
        cell_t *cell = &q->data[head & QUEUE_MASK];
        uint32_t seq = cell->seq;
        int32_t diff = seq - head;
        if (0 == diff) {
            if (__atomic_compare_exchange_n(&q->head, &head, head+1, 1, __ATOMIC_RELAXED, __ATOMIC_RELAXED)) {
            //q->head    = head + 1;
            cell->size = len;
            for (int d=60; d<len; d+=64) {
                __builtin_prefetch(cell->buf+d, 1, 1);
            }
            memcpy(cell->buf, c, len);
            cell->seq  = head + 1;
            return SUCCESS;
            }
        } else if (diff < 0) {
            return BUFFER_FULL;
        } else {
            head = q->head;
        }
    }
    return BUFFER_FULL;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    uint32_t tail = q->tail;
    cell_t *cell = &q->data[tail & QUEUE_MASK];
    uint32_t seq = cell->seq;
    int32_t diff = seq - (tail+1);

    if (0 == diff) {
        q->tail++;
        *len = cell->size;
        memcpy(c, cell->buf, *len);
        cell->seq  = tail + QUEUE_MASK + 1;
        return SUCCESS;
    }
    return BUFFER_EMPTY;
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
