#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sched.h>
#include <assert.h>
#include <locale.h>

#include "fifo5.h"

#define TEST_ITER 10
#define CPU_FREQ  2401000.0
#define TEST_SIZE 1 * 1000 * 1000

static int msg_size = 256;
static struct queue_t *sendq, *recvq;
pthread_barrier_t barrier;

void *consumer(void *arg)
{
    uint64_t i, j;
    uint64_t start, stop, diff;
    cpu_set_t cur_mask;
    int slen, rlen;
    slen = rlen = msg_size;
    char sbuf[BUF_SIZE] = {0};
    char rbuf[BUF_SIZE] = {0};

    CPU_ZERO(&cur_mask);
    CPU_SET(2, &cur_mask);
    if (sched_setaffinity(0, sizeof(cur_mask), &cur_mask) < 0) {
        printf("Error: sched_setaffinity\n");
        return NULL;
    }

    pthread_barrier_wait(&barrier);
    diff = 0;

    for (j = 0; j < TEST_ITER; j++) {
        pthread_barrier_wait(&barrier);
        start = read_tsc();

        for (i = 0; i < TEST_SIZE; i++) {
            while( dequeue(sendq, sbuf, &slen) != 0 );
            while( enqueue(recvq, rbuf, rlen ) != 0 );
            //assert(slen == msg_size);
        }

        stop = read_tsc();
        diff += stop - start;
    }
    printf("consumer lat cycles/op: %ld us/op: %lf ops/s: %'.0lf\n",
            diff / (TEST_SIZE * 2 * TEST_ITER),
            1.0e3 * (diff / CPU_FREQ) / (TEST_SIZE * 2 * TEST_ITER),
            1.0e3 * (TEST_SIZE * 2 * TEST_ITER) / (diff / CPU_FREQ));

    pthread_barrier_wait(&barrier);
    diff = 0;

    for (j = 0; j < TEST_ITER; j++) {
        pthread_barrier_wait(&barrier);
        start = read_tsc();

        for (i = 0; i < TEST_SIZE; i++) {
            while( dequeue(sendq, sbuf, &slen) != 0 );
        }

        stop = read_tsc();
        diff += stop - start;
    }
    printf("consumer bw cycles/op: %ld us/op: %lf ops/s: %'.0lf\n",
            diff / (TEST_SIZE * TEST_ITER),
            1.0e3 * (diff / CPU_FREQ) / (TEST_SIZE * TEST_ITER),
            1.0e3 * (TEST_SIZE * TEST_ITER) / (diff / CPU_FREQ));

    pthread_barrier_wait(&barrier);
    return NULL;
}

void *producer(void)
{
    uint64_t i, j;
    uint64_t start, stop, diff;
    cpu_set_t	cur_mask;
    int slen, rlen;
    slen = rlen = msg_size;
    char sbuf[BUF_SIZE] = {0};
    char rbuf[BUF_SIZE] = {0};

    CPU_ZERO(&cur_mask);
    CPU_SET(1, &cur_mask);
    if (sched_setaffinity(0, sizeof(cur_mask), &cur_mask) < 0) {
        printf("Error: sched_setaffinity\n");
        return NULL;
    }

    pthread_barrier_wait(&barrier);
    diff = 0;

    for (j = 0; j < TEST_ITER; j++) {
        pthread_barrier_wait(&barrier);
        start = read_tsc();

        for (i = 0; i < TEST_SIZE; i++) {
            while( enqueue(sendq, sbuf, slen ) != 0 );
            while( dequeue(recvq, rbuf, &rlen) != 0 );
            //assert(slen == msg_size);
        }

        stop = read_tsc();
        diff += stop - start;
    }
    printf("producer lat cycles/op: %ld us/op: %lf ops/s: %'.0lf\n",
            diff / (TEST_SIZE * 2 * TEST_ITER),
            1.0e3 * (diff / CPU_FREQ) / (TEST_SIZE * 2 * TEST_ITER),
            1.0e3 * (TEST_SIZE * 2 * TEST_ITER) / (diff / CPU_FREQ));

    pthread_barrier_wait(&barrier);
    diff = 0;

    for (j = 0; j < TEST_ITER; j++) {
        pthread_barrier_wait(&barrier);
        start = read_tsc();

        for (i = 0; i < TEST_SIZE; i++) {
            while( enqueue(sendq, sbuf, slen) != 0 );
        }

        stop = read_tsc();
        diff += stop - start;
    }
    printf("producer bw cycles/op: %ld us/op: %lf ops/s: %'.0lf\n",
            diff / (TEST_SIZE * TEST_ITER),
            1.0e3 * (diff / CPU_FREQ) / (TEST_SIZE * TEST_ITER),
            1.0e3 * (TEST_SIZE * TEST_ITER) / (diff / CPU_FREQ));

    pthread_barrier_wait(&barrier);
    return NULL;
}

int main(int argc, char *argv[])
{
    int	error;
    pthread_t consumer_thread;

    if (argc > 1)   {
        msg_size = atoi(argv[1]);
    }

    setlocale(LC_NUMERIC, "");
    //printf("sizeof cell_t: %ld, queue_t: %ld\n", sizeof(struct cell_t), sizeof(struct queue_t));

    srand((unsigned int)read_tsc());

    sendq = aligned_alloc(PAGE_SIZE, sizeof(struct queue_t));
    recvq = aligned_alloc(PAGE_SIZE, sizeof(struct queue_t));
    queue_init(sendq);
    queue_init(recvq);

    error = pthread_barrier_init(&barrier, NULL, 2);
    if (error != 0) {
        perror("BW");
        return 1;
    }

    error = pthread_create(&consumer_thread, NULL, consumer, NULL);
    if (error != 0) {
        perror("BW");
        return 1;
    }

    producer();
    //printf("Done!\n");

    queue_fini(sendq);
    queue_fini(recvq);
    free(sendq);
    free(recvq);
    return 0;
}
