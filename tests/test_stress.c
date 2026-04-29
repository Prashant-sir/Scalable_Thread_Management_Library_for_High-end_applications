/**
 * @file test_stress.c
 * @brief Stress tests for the thread pool
 *
 * Tests:
 *   - Thousands of concurrent tasks
 *   - Performance measurement
 *   - Memory stability under load
 *   - Worker thread distribution
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "core/thread_pool.h"
#include "utils/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#define NUM_WORKERS     8
#define NUM_TASKS_1K    1000
#define NUM_TASKS_10K   10000
#define NUM_TASKS_100K  100000

static atomic_int completed_count;

/* Fast task - minimal work */
static void fast_counter_task(void *arg) {
    (void)arg;
    atomic_fetch_add(&completed_count, 1);
}

/* Medium task - small computation */
static void medium_task(void *arg) {
    int id = *(int *)arg;
    volatile int sum = 0;
    for (int i = 0; i < 1000; i++) {
        sum += i * id;
    }
    (void)sum;
    atomic_fetch_add(&completed_count, 1);
}

/* Task with variable duration */
static void variable_duration_task(void *arg) {
    int duration_us = *(int *)arg;
    if (duration_us > 0) {
        usleep((useconds_t)duration_us);
    }
    atomic_fetch_add(&completed_count, 1);
}

static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

static void print_stats(thread_pool_t *pool, double elapsed_ms, int num_tasks) {
    double tasks_per_sec = (num_tasks / elapsed_ms) * 1000.0;
    double avg_latency_us = (elapsed_ms * 1000.0) / num_tasks;

    printf("  Completed:    %d tasks\n", num_tasks);
    printf("  Time:         %.2f ms\n", elapsed_ms);
    printf("  Throughput:   %.0f tasks/sec\n", tasks_per_sec);
    printf("  Avg Latency:  %.2f us/task\n", avg_latency_us);
}

static void test_1k_fast_tasks(void) {
    printf("\n[STRESS] 1,000 fast tasks\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    double start = get_time_ms();

    for (int i = 0; i < NUM_TASKS_1K; i++) {
        if (!thread_pool_submit(&pool, fast_counter_task, NULL)) {
            printf("  FAILED: task submission failed at %d\n", i);
            thread_pool_destroy(&pool);
            return;
        }
    }

    thread_pool_wait_all(&pool);
    double elapsed = get_time_ms() - start;

    if (atomic_load(&completed_count) != NUM_TASKS_1K) {
        printf("  FAILED: expected %d, got %d\n",
               NUM_TASKS_1K, atomic_load(&completed_count));
    } else {
        print_stats(&pool, elapsed, NUM_TASKS_1K);
        printf("  PASSED\n");
    }

    thread_pool_destroy(&pool);
}

static void test_10k_fast_tasks(void) {
    printf("\n[STRESS] 10,000 fast tasks\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    double start = get_time_ms();

    for (int i = 0; i < NUM_TASKS_10K; i++) {
        thread_pool_submit(&pool, fast_counter_task, NULL);
    }

    thread_pool_wait_all(&pool);
    double elapsed = get_time_ms() - start;

    if (atomic_load(&completed_count) != NUM_TASKS_10K) {
        printf("  FAILED: expected %d, got %d\n",
               NUM_TASKS_10K, atomic_load(&completed_count));
    } else {
        print_stats(&pool, elapsed, NUM_TASKS_10K);
        printf("  PASSED\n");
    }

    thread_pool_destroy(&pool);
}

static void test_10k_computation_tasks(void) {
    printf("\n[STRESS] 10,000 computation tasks\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    int *ids = malloc(NUM_TASKS_10K * sizeof(int));
    for (int i = 0; i < NUM_TASKS_10K; i++) {
        ids[i] = i;
    }

    double start = get_time_ms();

    for (int i = 0; i < NUM_TASKS_10K; i++) {
        thread_pool_submit(&pool, medium_task, &ids[i]);
    }

    thread_pool_wait_all(&pool);
    double elapsed = get_time_ms() - start;

    if (atomic_load(&completed_count) != NUM_TASKS_10K) {
        printf("  FAILED: expected %d, got %d\n",
               NUM_TASKS_10K, atomic_load(&completed_count));
    } else {
        print_stats(&pool, elapsed, NUM_TASKS_10K);
        printf("  PASSED\n");
    }

    free(ids);
    thread_pool_destroy(&pool);
}

static void test_mixed_duration_tasks(void) {
    printf("\n[STRESS] 1,000 mixed-duration tasks\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    int *durations = malloc(NUM_TASKS_1K * sizeof(int));
    srand((unsigned)time(NULL));

    /* Random durations between 0 and 5000 us */
    for (int i = 0; i < NUM_TASKS_1K; i++) {
        durations[i] = rand() % 5000;
    }

    double start = get_time_ms();

    for (int i = 0; i < NUM_TASKS_1K; i++) {
        thread_pool_submit(&pool, variable_duration_task, &durations[i]);
    }

    thread_pool_wait_all(&pool);
    double elapsed = get_time_ms() - start;

    if (atomic_load(&completed_count) != NUM_TASKS_1K) {
        printf("  FAILED: expected %d, got %d\n",
               NUM_TASKS_1K, atomic_load(&completed_count));
    } else {
        print_stats(&pool, elapsed, NUM_TASKS_1K);
        printf("  PASSED\n");
    }

    free(durations);
    thread_pool_destroy(&pool);
}

static void test_worker_distribution(void) {
    printf("\n[STRESS] Worker distribution test\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Submit many tasks */
    for (int i = 0; i < NUM_TASKS_10K; i++) {
        thread_pool_submit(&pool, fast_counter_task, NULL);
    }

    thread_pool_wait_all(&pool);

    /* Print per-worker stats */
    printf("  Worker task distribution:\n");
    for (size_t i = 0; i < pool.worker_count; i++) {
        int count = atomic_load(&pool.workers[i].task_count);
        printf("    Worker %zu: %d tasks (%.1f%%)\n",
               i, count, (count * 100.0) / NUM_TASKS_10K);
    }

    if (atomic_load(&completed_count) != NUM_TASKS_10K) {
        printf("  FAILED\n");
    } else {
        printf("  PASSED\n");
    }

    thread_pool_destroy(&pool);
}

int main(void) {
    printf("\n========================================\n");
    printf("  Stress Tests - Thread Pool\n");
    printf("========================================\n");
    printf("  Workers: %d\n", NUM_WORKERS);
    printf("  Platform: Linux (pthreads)\n");

    logger_init();

    test_1k_fast_tasks();
    test_10k_fast_tasks();
    test_10k_computation_tasks();
    test_mixed_duration_tasks();
    test_worker_distribution();

    printf("\nAll stress tests completed!\n\n");
    return 0;
}
