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
    uint32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* Vyukov Bounded MPMC Queue */
struct queue_t {
	volatile uint32_t head CACHE_ALIGNED;
	volatile uint32_t tail CACHE_ALIGNED;
    struct cell_t data[QUEUE_SIZE] CACHE_ALIGNED;
} CACHE_ALIGNED;
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
    int i;
	memset(q, 0, sizeof(struct queue_t));
    for (i=0; i<QUEUE_SIZE; i++) {
        q->data[i].seq = i;
    }
}

void queue_fini(struct queue_t *q)
{
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    struct cell_t *cell;
    uint32_t tail = q->tail;
    for (;;) {
        cell = &q->data[tail & QUEUE_MASK];
        uint32_t seq = cell->seq;
        uint32_t dif = seq - tail;

        if (dif == 0) {
            if (__sync_bool_compare_and_swap(&q->tail, tail, tail+1)) {
                break;
            }
        } else if (dif < 0) {
            return BUFFER_FULL;
        } else {
            tail = q->tail;
        }
    }

    for (int d=60; d<len; d+=64) {
        __builtin_prefetch(cell->buf+d, 1, 1);
    }
    memcpy(cell->buf, c, len);
    cell->size = len;
    cell->seq  = tail + 1;

	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    struct cell_t *cell;
    uint32_t head = q->head;
    for (;;) {
        cell = &q->data[head & QUEUE_MASK];
        uint32_t seq = cell->seq;
        uint32_t dif = seq - (head+1);

        if (dif == 0) {
            if (__sync_bool_compare_and_swap(&q->head, head, head+1)) {
                break;
            }
        } else if (dif < 0) {
            return BUFFER_EMPTY;
        } else {
            head = q->head;
        }
    }
    *len = cell->size;
    memcpy(c, cell->buf, *len);
    cell->size = 0;
    cell->seq  = head + QUEUE_MASK + 1;

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
