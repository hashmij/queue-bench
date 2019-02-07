#ifndef _FBOX_H_
#define _FBOX_H_

#include <unistd.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string.h>
#include <stdint.h>


#define FBOX_FULL 1
#define FBOX_NULL 0
#define FBOX_BUSY 2

#define SUCCESS 0
#define BUFFER_FULL -1
#define BUFFER_EMPTY -2

#define PAGE_SIZE 4096
#define CACHE_ALIGNED  __attribute__ ((aligned(128)))

#define BUF_SIZE (4096-8)
struct queue_t {
	volatile uint32_t flag;
	volatile uint32_t size;
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
    if (f->flag)
		return BUFFER_FULL;

    f->size = len;
    memcpy(f->buf, c, len);
    f->flag = FBOX_FULL;

	return SUCCESS;
}

inline int dequeue(struct queue_t *f, char *c, int *len)
{
    if (!f->flag)
		return BUFFER_EMPTY;

    *len = f->size;
    memcpy(c, f->buf, f->size);
    f->flag = FBOX_NULL;

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
