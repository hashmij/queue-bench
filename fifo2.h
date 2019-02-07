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

#define BUF_SIZE (4096-4)
struct cell_t {
    uint32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* Lamport's Original */
struct queue_t {
	volatile uint32_t head;
	volatile uint32_t tail;
    struct cell_t *data;
};
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
    if ((q->tail + 1) % QUEUE_SIZE == q->head)
        return BUFFER_FULL;

    cell_t *e = &q->data[q->tail];
    e->size = len;
    memcpy(e->buf, c, len);

	q->tail = (q->tail + 1) % QUEUE_SIZE;
	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    if (q->tail == q->head)
        return BUFFER_EMPTY;

    cell_t *e = &q->data[q->head];
    *len = e->size;
    memcpy(c, e->buf, *len);

	q->head = (q->head + 1) % QUEUE_SIZE;
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
