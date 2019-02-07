#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <locale.h>

#include "mpmc1.h"

#define MAX_THREADS 32
#define TEST_ITER   1 * 1000L
#define WINDOW_SIZE 1 * 1L
#define CPU_FREQ    2401000.0

static int nthreads = 2;
static int msg_size = 4;
static struct queue_t *q;
pthread_barrier_t barrier;

void *runner(void *arg)
{
    uint64_t i, j, k;
    uint64_t start, stop, diff;
    cpu_set_t cur_mask;
    int len = msg_size;
    const int tid = (size_t)arg;
    const int nth = nthreads;
    char *buf = NULL;

    CPU_ZERO(&cur_mask);
    CPU_SET(tid, &cur_mask);
    if (sched_setaffinity(0, sizeof(cur_mask), &cur_mask) < 0) {
        printf("Error: sched_setaffinity\n");
        return NULL;
    }

    if (!tid) {
        buf = aligned_alloc(PAGE_SIZE, BUF_SIZE * nth);
        assert(buf);
        memset(buf, tid, BUF_SIZE * nth);
    } else {
        buf = aligned_alloc(PAGE_SIZE, BUF_SIZE);
        assert(buf);
        memset(buf, tid, BUF_SIZE);
    }

    pthread_barrier_wait(&barrier);
    diff = 0;

    for (j = 0; j < TEST_ITER; j++) {
        pthread_barrier_wait(&barrier);
        start = read_tsc();

        if (!tid) {
            for (i = 0; i < WINDOW_SIZE; i++) {
                for (k = 1; k < nth; k++) {
                    while ( dequeue(&q[0], &buf[k * BUF_SIZE], &len) != 0 );
                }
            }
        } else {
            for (i = 0; i < WINDOW_SIZE; i++) {
                while( enqueue(&q[0], buf, len) != 0 );
            }
        }

        stop = read_tsc();
        diff += stop - start;
    }

    if (!tid) {
        printf("cycles/op: %ld us/op: %lf ops/s: %'.0lf\n",
            diff / (WINDOW_SIZE * TEST_ITER),
            1.0e3 * (diff / CPU_FREQ) / (WINDOW_SIZE * TEST_ITER),
            1.0e3 * (WINDOW_SIZE * TEST_ITER) / (diff / CPU_FREQ));
    }

    pthread_barrier_wait(&barrier);
    free(buf);
    return NULL;
}

int main(int argc, char *argv[])
{
    int	error, i;
    pthread_t threads[MAX_THREADS];

    if (argc > 1)   {
        nthreads = atoi(argv[1]);
        assert(nthreads > 1);
    }

    if (argc > 2)   {
        msg_size = atoi(argv[2]);
        assert(msg_size > 0);
    }

    setlocale(LC_NUMERIC, "");
    printf("bcast:: nthreads: %d msg_size: %d ", nthreads, msg_size);

    srand((unsigned int)read_tsc());

    error = pthread_barrier_init(&barrier, NULL, nthreads);
    assert(!error);

    q = aligned_alloc(PAGE_SIZE, sizeof(struct queue_t) * nthreads);
    assert(q);

    for (i = 0; i < nthreads; i++) {
        queue_init(&q[i]);
    }

    for (i = 0; i < nthreads; i++) {
        error = pthread_create(&threads[i], NULL, runner, (void*)(size_t)i);
        assert(!error);
    }

    for (i = 0; i < nthreads; i++) {
        error = pthread_join(threads[i], NULL);
        assert(!error);
    }

    for (i=0; i<nthreads; i++) {
        queue_fini(&q[i]);
    }
    free(q);

    return 0;
}
