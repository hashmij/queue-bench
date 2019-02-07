#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <locale.h>

#include "mpmc2.h"

#define MAX_THREADS 32
#define TEST_ITER   10
#define TEST_SIZE   1L * 1000 * 1000
#define CPU_FREQ    2401000.0

static int nthreads = 2;
static int msg_size = 4;
static struct queue_t *q;
pthread_barrier_t barrier;
uint64_t diffs[MAX_THREADS];

void *runner(void *arg)
{
    uint64_t i, j;
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

    buf = aligned_alloc(PAGE_SIZE, BUF_SIZE);
    assert(buf);
    memset(buf, tid, BUF_SIZE);

    pthread_barrier_wait(&barrier);
    diff = 0;

    for (j = 0; j < TEST_ITER; j++) {
        pthread_barrier_wait(&barrier);
        start = read_tsc();

        if (tid % 2) {
            for (i = 0; i < TEST_SIZE; i++) {
                while( enqueue(q, buf, len) != 0 );
            }
        } else {
            for (i = 0; i < TEST_SIZE; i++) {
                while( dequeue(q, buf, &len) != 0 );
            }
        }

        stop = read_tsc();
        diff += stop - start;
    }

    diffs[tid] = diff;
    pthread_barrier_wait(&barrier);

    if (!tid) {
        for (diff=0, i=0; i<nth; i++) {
            diff += diffs[i];
        }
        diff /= nth;
        printf("mbw cycles/op: %ld us/op: %lf ops/s: %'.0lf mops/s: %'.0lf\n",
                diff / (TEST_SIZE * TEST_ITER),
                1.0e3 * (diff / CPU_FREQ) / (TEST_SIZE * TEST_ITER),
                1.0e3 * (TEST_SIZE * TEST_ITER) / (diff / CPU_FREQ),
                1.0e3 * (TEST_SIZE * TEST_ITER) / (diff / CPU_FREQ) * nth);
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
        assert(nthreads % 2 == 0);
    }

    if (argc > 2)   {
        msg_size = atoi(argv[2]);
        assert(msg_size > 0);
    }

    setlocale(LC_NUMERIC, "");
    printf("mbw:: nthreads: %d msg_size: %d \n", nthreads, msg_size);

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
