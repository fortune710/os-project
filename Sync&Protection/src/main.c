#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>
#include "sync.h"

typedef struct {
    int iterations;
    int failures;
    int consume_total;
} thread_args_t;

void* producer_thread_demo(void* arg) {
    (void)arg;
    for (int i = 1; i <= 5; i++) {
        if (sync_produce(i) == 0) {
            printf("[Producer] Successfully added item: %d\n", i);
        } else {
            printf("[Producer] Failed to add item.\n");
        }
        sleep(1);
    }
    return NULL;
}

void* consumer_thread_demo(void* arg) {
    (void)arg;
    int item;
    for (int i = 1; i <= 5; i++) {
        if (sync_consume(&item) == 0) {
            printf("[Consumer] Successfully removed item: %d\n", item);
        } else {
            printf("[Consumer] Failed to remove item.\n");
        }
        sleep(1);
    }
    return NULL;
}

void* producer_thread_bench(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    for (int i = 0; i < args->iterations; i++) {
        if (sync_produce(i) != 0) {
            args->failures++;
        }
    }
    return NULL;
}

void* consumer_thread_bench(void* arg) {
    thread_args_t* args = (thread_args_t*)arg;
    int item = 0;
    for (int i = 0; i < args->iterations; i++) {
        if (sync_consume(&item) != 0) {
            args->failures++;
        } else {
            args->consume_total += item;
        }
    }
    return NULL;
}

static double elapsed_ms(const struct timespec* start, const struct timespec* end) {
    long sec = end->tv_sec - start->tv_sec;
    long nsec = end->tv_nsec - start->tv_nsec;
    return (double)sec * 1000.0 + (double)nsec / 1000000.0;
}

static int run_demo(void) {
    printf("--- Starting MOSS Synchronization Test ---\n");

    if (sync_pc_init() != 0) {
        printf("Error: Failed to initialize subsystem.\n");
        return -1;
    }

    pthread_t prod, cons;

    pthread_create(&prod, NULL, producer_thread_demo, NULL);
    pthread_create(&cons, NULL, consumer_thread_demo, NULL);

    pthread_join(prod, NULL);
    pthread_join(cons, NULL);

    printf("--- Test Complete ---\n");
    return 0;
}

static int run_benchmark(void) {
    const int workloads[] = {1000, 10000, 100000};
    const int num_workloads = (int)(sizeof(workloads) / sizeof(workloads[0]));

    printf("iterations,total_ms,avg_op_us,producer_failures,consumer_failures\n");
    for (int i = 0; i < num_workloads; i++) {
        const int iterations = workloads[i];
        pthread_t prod, cons;
        thread_args_t prod_args = {iterations, 0, 0};
        thread_args_t cons_args = {iterations, 0, 0};
        struct timespec start;
        struct timespec end;
        double total_ms;
        double avg_op_us;

        if (sync_pc_init() != 0 || sync_pc_reset() != 0) {
            printf("benchmark_error: failed to initialize sync subsystem\n");
            return -1;
        }

        clock_gettime(CLOCK_MONOTONIC, &start);
        pthread_create(&prod, NULL, producer_thread_bench, &prod_args);
        pthread_create(&cons, NULL, consumer_thread_bench, &cons_args);
        pthread_join(prod, NULL);
        pthread_join(cons, NULL);
        clock_gettime(CLOCK_MONOTONIC, &end);

        total_ms = elapsed_ms(&start, &end);
        avg_op_us = (total_ms * 1000.0) / (2.0 * (double)iterations);
        printf("%d,%.3f,%.3f,%d,%d\n",
               iterations,
               total_ms,
               avg_op_us,
               prod_args.failures,
               cons_args.failures);
    }

    return 0;
}

int main(int argc, char** argv) {
    if (argc > 1 && strcmp(argv[1], "bench") == 0) {
        return run_benchmark();
    }

    if (argc > 1 && strcmp(argv[1], "demo") != 0) {
        printf("Usage: %s [demo|bench]\n", argv[0]);
        return -1;
    }

    return run_demo();
}
