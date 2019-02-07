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

#define BATCH_SIZE 1
#define QUEUE_SIZE (256) 
#define QUEUE_MASK (QUEUE_SIZE-1) 
#define CACHE_ALIGNED  __attribute__ ((aligned(128)))

#define BUF_SIZE (4096-4)
struct cell_t {
    uint32_t size;
    char buf[BUF_SIZE];
};
typedef struct cell_t cell_t;

/* MCBufferRing */
struct queue_t {
    /* Shared variables */
	volatile int32_t read;
	volatile int32_t write;
    int64_t pad0;
    /* Consumer variables */
    int32_t localWrite;
    int32_t nextRead;
    int32_t rBatch;
    int32_t pad1;
    /* Producer variables */
    int32_t localRead;
    int32_t nextWrite;
    int32_t wBatch;
    int32_t pad2;
    /* Shared data */
    struct cell_t *data;
};
typedef struct queue_t queue_t;

void queue_init(struct queue_t *q)
{
	memset(q, 0, sizeof(struct queue_t));
    q->data = malloc(QUEUE_SIZE * sizeof(struct cell_t));
    memset(q->data, 0, QUEUE_SIZE * sizeof(struct cell_t));
}

void queue_fini(struct queue_t *q)
{
    free(q->data);
}

inline int enqueue(struct queue_t *q, const char *c, int len)
{
    int anw = (q->nextWrite + 1) % QUEUE_SIZE;
    if (anw == q->localRead) {
        if (anw == q->read) {
            return BUFFER_FULL;
        }
        q->localRead = q->read;
    }

    cell_t *e = &q->data[q->nextWrite];
    e->size = len;
    memcpy(e->buf, c, len);

    q->nextWrite = anw;
    q->wBatch++;
    if (q->wBatch >= BATCH_SIZE) {
        q->write = q->nextWrite;
        q->wBatch = 0;
    }
	return SUCCESS;
}

inline int dequeue(struct queue_t *q, char *c, int *len)
{
    if (q->nextRead == q->localWrite) {
        if (q->nextRead == q->write) {
            return BUFFER_EMPTY;
        }
        q->localWrite = q->write;
    }

    cell_t *e = &q->data[q->nextRead];
    *len = e->size;
    memcpy(c, e->buf, *len);

    q->nextRead = (q->nextRead + 1) % QUEUE_SIZE;
    q->rBatch++;
    if (q->rBatch >= BATCH_SIZE) {
        q->read = q->nextRead;
        q->rBatch = 0;
    }
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
