/**
 * @file demo.c
 * @brief Demonstration of the thread pool library
 *
 * This example shows how to use the thread pool in real scenarios:
 *   - Parallel computation
 *   - Simulated web request handling
 *   - Concurrent file processing
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "core/thread_pool.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

/* ============================ */
/* Example 1: Parallel Sum      */
/* ============================ */

typedef struct {
    int *data;
    int start;
    int end;
    long *result;
} sum_range_t;

static void partial_sum(void *arg) {
    sum_range_t *range = (sum_range_t *)arg;
    long sum = 0;
    for (int i = range->start; i < range->end; i++) {
        sum += range->data[i];
    }
    *range->result = sum;
    printf("  Sum range [%d, %d) = %ld\n", range->start, range->end, sum);
}

static void demo_parallel_sum(void) {
    printf("\n[Demo 1] Parallel Sum Computation\n");
    printf("  Calculating sum of 1,000,000 numbers using 4 workers\n\n");

    const int ARRAY_SIZE = 1000000;
    const int NUM_WORKERS = 4;

    /* Create array */
    int *numbers = malloc(ARRAY_SIZE * sizeof(int));
    for (int i = 0; i < ARRAY_SIZE; i++) {
        numbers[i] = i + 1;
    }

    /* Expected sum: n*(n+1)/2 = 1000000*1000001/2 */
    long expected_sum = (long)ARRAY_SIZE * (ARRAY_SIZE + 1) / 2;

    /* Setup thread pool */
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Divide work among workers */
    long partial_results[NUM_WORKERS];
    sum_range_t ranges[NUM_WORKERS];
    int chunk_size = ARRAY_SIZE / NUM_WORKERS;

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < NUM_WORKERS; i++) {
        partial_results[i] = 0;
        ranges[i].data = numbers;
        ranges[i].start = i * chunk_size;
        ranges[i].end = (i == NUM_WORKERS - 1) ? ARRAY_SIZE : (i + 1) * chunk_size;
        ranges[i].result = &partial_results[i];
        thread_pool_submit(&pool, partial_sum, &ranges[i]);
    }

    thread_pool_wait_all(&pool);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    /* Combine results */
    long total_sum = 0;
    for (int i = 0; i < NUM_WORKERS; i++) {
        total_sum += partial_results[i];
    }

    printf("\n  Expected sum: %ld\n", expected_sum);
    printf("  Actual sum:   %ld\n", total_sum);
    printf("  Match:        %s\n", (total_sum == expected_sum) ? "YES" : "NO");
    printf("  Time:         %.2f ms\n", elapsed_ms);

    thread_pool_destroy(&pool);
    free(numbers);
}

/* ============================ */
/* Example 2: Web Server Sim    */
/* ============================ */

typedef struct {
    int request_id;
    int processing_time_ms;
} request_t;

static void handle_request(void *arg) {
    request_t *req = (request_t *)arg;
    printf("  [Request %d] Started (will take %d ms)\n",
           req->request_id, req->processing_time_ms);
    usleep(req->processing_time_ms * 1000);
    printf("  [Request %d] Completed\n", req->request_id);
}

static void demo_web_server(void) {
    printf("\n[Demo 2] Simulated Web Request Handler\n");
    printf("  Handling 8 requests with 3 worker threads\n\n");

    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(3);
    thread_pool_init(&pool, config);

    request_t requests[] = {
        {1, 100}, {2, 50}, {3, 200}, {4, 30},
        {5, 150}, {6, 80}, {7, 60}, {8, 120}
    };

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    for (int i = 0; i < 8; i++) {
        thread_pool_submit(&pool, handle_request, &requests[i]);
    }

    thread_pool_wait_all(&pool);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    printf("\n  All requests handled in %.0f ms\n", elapsed_ms);
    printf("  Sequential time would be: %d ms\n", 100+50+200+30+150+80+60+120);

    thread_pool_destroy(&pool);
}

/* ============================ */
/* Example 3: Matrix Multiply   */
/* ============================ */

typedef struct {
    int row;
    int size;
    const int *matrix_a;
    const int *matrix_b;
    int *result;
} multiply_row_t;

static void multiply_row(void *arg) {
    multiply_row_t *work = (multiply_row_t *)arg;
    int row = work->row;
    int size = work->size;

    for (int col = 0; col < size; col++) {
        int sum = 0;
        for (int k = 0; k < size; k++) {
            sum += work->matrix_a[row * size + k] * work->matrix_b[k * size + col];
        }
        work->result[row * size + col] = sum;
    }
}

static void demo_matrix_multiply(void) {
    printf("\n[Demo 3] Parallel Matrix Multiplication\n");
    printf("  Multiplying two 100x100 matrices\n\n");

    const int SIZE = 100;
    const int NUM_WORKERS = 4;

    /* Allocate matrices */
    int *a = calloc(SIZE * SIZE, sizeof(int));
    int *b = calloc(SIZE * SIZE, sizeof(int));
    int *c = calloc(SIZE * SIZE, sizeof(int));

    /* Initialize with simple values */
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            a[i * SIZE + j] = i + j;
            b[i * SIZE + j] = (i == j) ? 1 : 0; /* Identity matrix */
        }
    }

    /* Setup pool */
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    /* Submit each row as a separate task */
    multiply_row_t *work_items = malloc(SIZE * sizeof(multiply_row_t));
    for (int i = 0; i < SIZE; i++) {
        work_items[i].row = i;
        work_items[i].size = SIZE;
        work_items[i].matrix_a = a;
        work_items[i].matrix_b = b;
        work_items[i].result = c;
        thread_pool_submit(&pool, multiply_row, &work_items[i]);
    }

    thread_pool_wait_all(&pool);

    clock_gettime(CLOCK_MONOTONIC, &end);
    double elapsed_ms = (end.tv_sec - start.tv_sec) * 1000.0 +
                        (end.tv_nsec - start.tv_nsec) / 1000000.0;

    /* Verify: C = A * I should equal A */
    int errors = 0;
    for (int i = 0; i < SIZE; i++) {
        for (int j = 0; j < SIZE; j++) {
            if (c[i * SIZE + j] != a[i * SIZE + j]) {
                errors++;
            }
        }
    }

    printf("  Result verification: %s (%d errors)\n",
           (errors == 0) ? "PASSED" : "FAILED", errors);
    printf("  Time: %.2f ms\n", elapsed_ms);

    thread_pool_print_stats(&pool);

    free(a);
    free(b);
    free(c);
    free(work_items);
    thread_pool_destroy(&pool);
}

/* ============================ */
/* Main                         */
/* ============================ */

int main(void) {
    printf("\n");
    printf("========================================\n");
    printf("  Thread Pool Library - Demo\n");
    printf("========================================\n");
    printf("\n  This demo showcases the thread pool library\n");
    printf("  with three practical examples:\n");
    printf("    1. Parallel sum computation\n");
    printf("    2. Simulated web request handling\n");
    printf("    3. Parallel matrix multiplication\n");

    demo_parallel_sum();
    demo_web_server();
    demo_matrix_multiply();

    printf("\n========================================\n");
    printf("  All demos completed successfully!\n");
    printf("========================================\n\n");

    return 0;
}
