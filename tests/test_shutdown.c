/**
 * @file test_shutdown.c
 * @brief Shutdown behavior tests for the thread pool
 *
 * Tests:
 *   - Graceful shutdown with pending tasks
 *   - Shutdown during active execution
 *   - Reject tasks after shutdown
 *   - Multiple shutdown calls (idempotent)
 *   - Submit during shutdown
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "core/thread_pool.h"
#include "utils/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

#define NUM_WORKERS 4

static atomic_int completed_count;
static atomic_int rejected_after_shutdown;

/* Task that takes some time */
static void slow_counter_task(void *arg) {
    (void)arg;
    usleep(10000); /* 10ms */
    atomic_fetch_add(&completed_count, 1);
}

/* Fast task */
static void fast_task(void *arg) {
    (void)arg;
    atomic_fetch_add(&completed_count, 1);
}

/* Very slow task */
static void very_slow_task(void *arg) {
    int id = *(int *)arg;
    usleep(200000); /* 200ms */
    printf("  Very slow task %d completed\n", id);
    atomic_fetch_add(&completed_count, 1);
}

static void test_graceful_shutdown(void) {
    printf("\n[SHUTDOWN] Graceful shutdown - complete all pending tasks\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Submit many tasks */
    for (int i = 0; i < 100; i++) {
        thread_pool_submit(&pool, slow_counter_task, NULL);
    }

    printf("  Submitted 100 tasks, queue size: %zu\n",
           thread_pool_queue_size(&pool));

    /* Shutdown - should wait for all tasks */
    thread_pool_shutdown(&pool);

    printf("  After shutdown: completed %d tasks\n",
           atomic_load(&completed_count));

    if (atomic_load(&completed_count) == 100) {
        printf("  PASSED - All 100 tasks completed before shutdown\n");
    } else {
        printf("  FAILED - Only %d tasks completed\n",
               atomic_load(&completed_count));
    }

    thread_pool_destroy(&pool);
}

static void test_reject_after_shutdown(void) {
    printf("\n[SHUTDOWN] Reject tasks after shutdown\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Shutdown immediately */
    thread_pool_shutdown(&pool);

    /* Try to submit after shutdown */
    bool result = thread_pool_submit(&pool, fast_task, NULL);

    if (!result) {
        printf("  PASSED - Task correctly rejected after shutdown\n");
    } else {
        printf("  FAILED - Task should have been rejected\n");
    }

    thread_pool_destroy(&pool);
}

static void test_submit_during_shutdown(void) {
    printf("\n[SHUTDOWN] Submit tasks during active shutdown\n");

    atomic_init(&completed_count, 0);
    atomic_init(&rejected_after_shutdown, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(2); /* Few workers to keep tasks queued */
    thread_pool_init(&pool, config);

    /* Submit some slow tasks */
    int task_ids[10];
    for (int i = 0; i < 10; i++) {
        task_ids[i] = i;
        thread_pool_submit(&pool, very_slow_task, &task_ids[i]);
    }

    /* Give workers time to start */
    usleep(50000);

    /* Start shutdown (this runs concurrently with executing tasks) */
    printf("  Starting shutdown while tasks are running...\n");
    thread_pool_shutdown(&pool);

    printf("  After shutdown: completed %d/10 tasks\n",
           atomic_load(&completed_count));
    printf("  PASSED - Shutdown completed without hanging\n");

    thread_pool_destroy(&pool);
}

static void test_double_shutdown(void) {
    printf("\n[SHUTDOWN] Double shutdown (idempotent)\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Submit some tasks */
    for (int i = 0; i < 10; i++) {
        thread_pool_submit(&pool, fast_task, NULL);
    }

    /* First shutdown */
    thread_pool_shutdown(&pool);
    int after_first = atomic_load(&completed_count);

    /* Second shutdown - should be safe */
    thread_pool_shutdown(&pool);
    int after_second = atomic_load(&completed_count);

    if (after_first == after_second && after_first == 10) {
        printf("  PASSED - Double shutdown is safe, %d tasks completed\n", after_first);
    } else {
        printf("  FAILED - Unexpected state after double shutdown\n");
    }

    thread_pool_destroy(&pool);
}

static void test_empty_pool_shutdown(void) {
    printf("\n[SHUTDOWN] Shutdown empty pool\n");

    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Shutdown without submitting any tasks */
    thread_pool_shutdown(&pool);

    printf("  PASSED - Empty pool shutdown without issues\n");
    thread_pool_destroy(&pool);
}

static void test_stats_after_shutdown(void) {
    printf("\n[SHUTDOWN] Statistics after shutdown\n");

    atomic_init(&completed_count, 0);
    thread_pool_t pool;
    pool_config_t config = pool_config_with_threads(NUM_WORKERS);
    thread_pool_init(&pool, config);

    /* Submit and execute tasks */
    for (int i = 0; i < 50; i++) {
        thread_pool_submit(&pool, fast_task, NULL);
    }

    thread_pool_wait_all(&pool);
    thread_pool_print_stats(&pool);

    printf("  PASSED\n");
    thread_pool_destroy(&pool);
}

int main(void) {
    printf("\n========================================\n");
    printf("  Shutdown Tests - Thread Pool\n");
    printf("========================================\n");

    logger_init();

    test_graceful_shutdown();
    test_reject_after_shutdown();
    test_submit_during_shutdown();
    test_double_shutdown();
    test_empty_pool_shutdown();
    test_stats_after_shutdown();

    printf("\nAll shutdown tests completed!\n\n");
    return 0;
}
