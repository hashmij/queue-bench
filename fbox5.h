#ifndef _FBOX_H_
#define _FBOX_H_

#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>


#define FBOX_FREE  0
#define FBOX_BUSY -1

#define SUCCESS 0
#define BUFFER_FULL -1
#define BUFFER_EMPTY -2

#define PAGE_SIZE 4096
#define CACHE_ALIGNED  __attribute__ ((aligned(64)))

#define BUF_SIZE (4096-4)
struct queue_t {
	volatile int32_t size;
    char buf[BUF_SIZE];
};

void queue_init(struct queue_t *f)
{
	memset(f, 0, sizeof(struct queue_t));
}

void queue_fini(struct queue_t *q)
{
}

inline int enqueue(struct queue_t *f, const char *c, int len)
{
    while(!__sync_bool_compare_and_swap(&f->size, FBOX_FREE, FBOX_BUSY));
    memcpy(f->buf, c, len);
    f->size = len;
    return SUCCESS;
}

inline int dequeue(struct queue_t *f, char *c, int *len)
{
    while((*len = f->size) <= 0);
    memcpy(c, f->buf, *len);
    f->size = FBOX_FREE;
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
