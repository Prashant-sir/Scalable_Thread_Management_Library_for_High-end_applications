/**
 * @file test_basic.c
 * @brief Basic functionality tests for the thread pool
 *
 * Tests:
 *   - Pool creation and destruction
 *   - Simple task submission
 *   - Task execution verification
 *   - Multiple task submission
 *   - Queue size queries
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "core/thread_pool.h"
#include "utils/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <time.h>

#define TEST(name) printf("  [TEST] %-40s ", name); fflush(stdout)
#define PASS() printf("PASSED\n")
#define FAIL(msg) do { printf("FAILED: %s\n", msg); exit(1); } while(0)

/* Test counters */
static atomic_int exec_count;
static atomic_int sum_result;

/* Simple task: increment counter */
static void increment_task(void *arg) {
    (void)arg;
    atomic_fetch_add(&exec_count, 1);
}

/* Task with argument: add value to sum */
static void add_task(void *arg) {
    int value = *(int *)arg;
    atomic_fetch_add(&sum_result, value);
}

/* Task that prints */
static void print_task(void *arg) {
    const char *msg = (const char *)arg;
    printf("[%s] ", msg);
}

/* Slow task to test concurrent execution */
static void slow_task(void *arg) {
    (void)arg;
    usleep(50000); /* 50ms */
    atomic_fetch_add(&exec_count, 1);
}

static void test_pool_create_destroy(void) {
    TEST("Pool create and destroy");

    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(4);

    if (thread_pool_init(&pool, config) != 0) {
        FAIL("pool_init failed");
    }

    if (thread_pool_is_running(&pool) != true) {
        FAIL("pool should be running");
    }

    thread_pool_destroy(&pool);
    PASS();
}

static void test_single_task(void) {
    TEST("Single task execution");

    atomic_init(&exec_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(2);

    thread_pool_init(&pool, config);
    thread_pool_submit(&pool, increment_task, NULL);
    thread_pool_wait_all(&pool);

    if (atomic_load(&exec_count) != 1) {
        FAIL("task was not executed");
    }

    thread_pool_destroy(&pool);
    PASS();
}

static void test_multiple_tasks(void) {
    TEST("Multiple task execution (100 tasks)");

    atomic_init(&exec_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(4);

    thread_pool_init(&pool, config);

    for (int i = 0; i < 100; i++) {
        if (!thread_pool_submit(&pool, increment_task, NULL)) {
            FAIL("task submission failed");
        }
    }

    thread_pool_wait_all(&pool);

    if (atomic_load(&exec_count) != 100) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 100, got %d", atomic_load(&exec_count));
        FAIL(buf);
    }

    thread_pool_destroy(&pool);
    PASS();
}

static void test_task_with_argument(void) {
    TEST("Task with argument");

    atomic_init(&sum_result, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(4);

    int values[10] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    thread_pool_init(&pool, config);

    for (int i = 0; i < 10; i++) {
        thread_pool_submit(&pool, add_task, &values[i]);
    }

    thread_pool_wait_all(&pool);

    /* Sum should be 55 (1+2+...+10) */
    if (atomic_load(&sum_result) != 55) {
        char buf[64];
        snprintf(buf, sizeof(buf), "expected 55, got %d", atomic_load(&sum_result));
        FAIL(buf);
    }

    thread_pool_destroy(&pool);
    PASS();
}

static void test_queue_size(void) {
    TEST("Queue size tracking");

    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(1); /* 1 worker to queue up tasks */

    thread_pool_init(&pool, config);

    size_t before = thread_pool_queue_size(&pool);
    if (before != 0) {
        FAIL("queue should be empty initially");
    }

    /* Submit slow tasks - they should queue up */
    for (int i = 0; i < 5; i++) {
        thread_pool_submit(&pool, slow_task, NULL);
    }

    /* Give time for worker to pick one up */
    usleep(10000);

    size_t during = thread_pool_queue_size(&pool);
    if (during == 0) {
        FAIL("queue should have pending tasks");
    }

    thread_pool_wait_all(&pool);

    size_t after = thread_pool_queue_size(&pool);
    if (after != 0) {
        FAIL("queue should be empty after completion");
    }

    thread_pool_destroy(&pool);
    PASS();
}

static void test_concurrent_execution(void) {
    TEST("Concurrent execution (4 workers)");

    atomic_init(&exec_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(4);

    /* Record start time */
    struct timespec start, end;
    clock_gettime(CLOCK_MONOTONIC, &start);

    thread_pool_init(&pool, config);

    /* Submit 4 slow tasks that can run concurrently */
    for (int i = 0; i < 4; i++) {
        thread_pool_submit(&pool, slow_task, NULL);
    }

    thread_pool_wait_all(&pool);

    clock_gettime(CLOCK_MONOTONIC, &end);
    long elapsed_ms = (end.tv_sec - start.tv_sec) * 1000 +
                      (end.tv_nsec - start.tv_nsec) / 1000000;

    /* With 4 workers, 4x50ms tasks should take ~50ms, not ~200ms */
    if (elapsed_ms > 200) {
        char buf[128];
        snprintf(buf, sizeof(buf), "took %ld ms, expected ~50ms (parallel)", elapsed_ms);
        FAIL(buf);
    }

    if (atomic_load(&exec_count) != 4) {
        FAIL("not all tasks executed");
    }

    thread_pool_destroy(&pool);
    PASS();
}

int main(void) {
    printf("\n========================================\n");
    printf("  Basic Thread Pool Tests\n");
    printf("========================================\n\n");

    logger_init();

    test_pool_create_destroy();
    test_single_task();
    test_multiple_tasks();
    test_task_with_argument();
    test_queue_size();
    test_concurrent_execution();

    printf("\nAll basic tests PASSED!\n\n");
    return 0;
}
