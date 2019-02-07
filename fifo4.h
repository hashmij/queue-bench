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
#define QUEUE_MASK (QUEUE_SIZE-1) 
#define CACHE_ALIGNED  __attribute__ ((aligned(64)))

#define BUF_SIZE (4096-4)
struct cell_t {
    int32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* Lamport's Optimized */
struct queue_t {
	volatile uint32_t head CACHE_ALIGNED;
	volatile uint32_t tail CACHE_ALIGNED;
    uint32_t head_c CACHE_ALIGNED;
    uint32_t tail_c CACHE_ALIGNED;
    //struct cell_t *data CACHE_ALIGNED;
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
    uint32_t head = q->head_c;
    uint32_t tail = q->tail;

    if (((tail + 1) & QUEUE_MASK) == head) {
        q->head_c = head = q->head;
    }
    if (((tail + 1) & QUEUE_MASK) == head) {
        return BUFFER_FULL;
    }

    cell_t *e = &q->data[q->tail_c];
    e->size = len;
    memcpy(e->buf, c, len);

	q->tail = (tail + 1) & QUEUE_MASK;
	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    uint32_t head = q->head;
    uint32_t tail = q->tail_c;

    if (tail == head) {
        q->tail_c = tail = q->tail;
    }
    if (tail == head) {
        return BUFFER_EMPTY;
    }

    cell_t *e = &q->data[head];
    *len = e->size;
    memcpy(c, e->buf, *len);

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
