/**
 * @file thread_pool.c
 * @brief High-performance thread pool implementation
 *
 * Worker threads run a continuous loop:
 *   1. Wait for a task from the queue (blocking)
 *   2. Execute the task
 *   3. Repeat until shutdown
 *
 * Graceful shutdown ensures all pending tasks complete
 * before workers terminate.
 */

#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "core/thread_pool.h"
#include "utils/logger.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>

/* ============================ */
/* Worker Thread Function       */
/* ============================ */

/**
 * @brief Main worker thread loop
 *
 * Each worker continuously pops tasks from the shared queue
 * and executes them until the pool is shutting down.
 */
static void *worker_thread(void *arg) {
    worker_t *worker = (worker_t *)arg;
    thread_pool_t *pool = worker->pool;

    log_debug("Worker %d started", worker->worker_id);

    while (1) {
        /* Check pool state before blocking on queue */
        if (atomic_load(&pool->state) == POOL_STATE_SHUTTING_DOWN &&
            safe_queue_is_empty(&pool->queue)) {
            log_debug("Worker %d: queue empty during shutdown, exiting", worker->worker_id);
            break;
        }

        /* Block until task available or shutdown */
        task_t *task = safe_queue_pop(&pool->queue);

        if (task == NULL) {
            /* Queue is shutdown and empty */
            log_debug("Worker %d: received NULL task, exiting", worker->worker_id);
            break;
        }

        /* Execute the task */
        atomic_store(&worker->active, true);
        task_execute(task);
        atomic_store(&worker->active, false);

        /* Update statistics */
        atomic_fetch_add(&pool->stats.tasks_completed, 1);
        atomic_fetch_add(&worker->task_count, 1);

        /* Clean up task */
        task_destroy(task);
    }

    atomic_store(&worker->active, false);
    log_debug("Worker %d finished (executed %d tasks)",
              worker->worker_id,
              atomic_load(&worker->task_count));

    return NULL;
}

/* ============================ */
/* Configuration Helpers        */
/* ============================ */

pool_config_t pool_config_default(void) {
    pool_config_t config;
    config.thread_count = (size_t)sysconf(_SC_NPROCESSORS_ONLN);
    if (config.thread_count == 0) {
        config.thread_count = 4; /* Fallback */
    }
    config.queue_capacity = 0; /* Unlimited */
    config.name = "default";
    return config;
}

pool_config_t pool_config_with_threads(size_t thread_count) {
    pool_config_t config = pool_config_default();
    config.thread_count = thread_count;
    return config;
}

/* ============================ */
/* Lifecycle Management         */
/* ============================ */

int thread_pool_init(thread_pool_t *pool, pool_config_t config) {
    if (pool == NULL || config.thread_count == 0) {
        return -1;
    }

    memset(pool, 0, sizeof(thread_pool_t));
    pool->config = config;
    atomic_init(&pool->state, POOL_STATE_INIT);

    /* Initialize statistics */
    atomic_init(&pool->stats.tasks_submitted, 0);
    atomic_init(&pool->stats.tasks_completed, 0);
    atomic_init(&pool->stats.tasks_rejected, 0);
    atomic_init(&pool->stats.peak_queue_size, 0);

    /* Initialize synchronization primitives */
    if (pthread_mutex_init(&pool->pool_mutex, NULL) != 0) {
        return -1;
    }

    if (pthread_cond_init(&pool->pool_cond, NULL) != 0) {
        pthread_mutex_destroy(&pool->pool_mutex);
        return -1;
    }

    /* Initialize the task queue */
    if (safe_queue_init(&pool->queue, config.queue_capacity) != 0) {
        pthread_cond_destroy(&pool->pool_cond);
        pthread_mutex_destroy(&pool->pool_mutex);
        return -1;
    }

    /* Allocate worker array */
    pool->workers = (worker_t *)calloc(config.thread_count, sizeof(worker_t));
    if (pool->workers == NULL) {
        safe_queue_destroy(&pool->queue);
        pthread_cond_destroy(&pool->pool_cond);
        pthread_mutex_destroy(&pool->pool_mutex);
        return -1;
    }

    pool->worker_count = config.thread_count;

    /* Create worker threads */
    for (size_t i = 0; i < config.thread_count; i++) {
        worker_t *worker = &pool->workers[i];
        worker->pool = pool;
        worker->worker_id = (int)i;
        atomic_init(&worker->task_count, 0);
        atomic_init(&worker->active, false);

        if (pthread_create(&worker->thread, NULL, worker_thread, worker) != 0) {
            /* Clean up already created threads */
            atomic_store(&pool->state, POOL_STATE_SHUTTING_DOWN);
            safe_queue_shutdown(&pool->queue);

            for (size_t j = 0; j < i; j++) {
                pthread_join(pool->workers[j].thread, NULL);
            }

            free(pool->workers);
            safe_queue_destroy(&pool->queue);
            pthread_cond_destroy(&pool->pool_cond);
            pthread_mutex_destroy(&pool->pool_mutex);
            return -1;
        }
    }

    atomic_store(&pool->state, POOL_STATE_RUNNING);
    log_info("Thread pool '%s' initialized with %zu workers",
             config.name, config.thread_count);

    return 0;
}

int thread_pool_init_default(thread_pool_t *pool) {
    return thread_pool_init(pool, pool_config_default());
}

void thread_pool_shutdown(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    pool_state_t expected = POOL_STATE_RUNNING;
    if (!atomic_compare_exchange_strong(&pool->state, &expected,
                                         POOL_STATE_SHUTTING_DOWN)) {
        /* Already shutting down or not running */
        return;
    }

    log_info("Thread pool shutting down...");

    /* Signal queue to wake all waiting workers */
    safe_queue_shutdown(&pool->queue);

    /* Wait for all workers to finish */
    for (size_t i = 0; i < pool->worker_count; i++) {
        pthread_join(pool->workers[i].thread, NULL);
    }

    atomic_store(&pool->state, POOL_STATE_SHUTDOWN);
    log_info("Thread pool shutdown complete");
}

void thread_pool_destroy(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    /* Shutdown if still running */
    if (atomic_load(&pool->state) == POOL_STATE_RUNNING) {
        thread_pool_shutdown(pool);
    }

    /* Clean up resources */
    if (pool->workers != NULL) {
        free(pool->workers);
        pool->workers = NULL;
    }

    safe_queue_destroy(&pool->queue);
    pthread_cond_destroy(&pool->pool_cond);
    pthread_mutex_destroy(&pool->pool_mutex);

    log_info("Thread pool destroyed");
}

/* ============================ */
/* Task Submission              */
/* ============================ */

bool thread_pool_submit(thread_pool_t *pool, task_func_t func, void *arg) {
    if (pool == NULL || func == NULL) {
        return false;
    }

    /* Reject if not running */
    if (atomic_load(&pool->state) != POOL_STATE_RUNNING) {
        atomic_fetch_add(&pool->stats.tasks_rejected, 1);
        return false;
    }

    /* Create task */
    task_t *task = task_create(func, arg);
    if (task == NULL) {
        atomic_fetch_add(&pool->stats.tasks_rejected, 1);
        return false;
    }

    /* Submit to queue */
    if (!safe_queue_push(&pool->queue, task)) {
        task_destroy(task);
        atomic_fetch_add(&pool->stats.tasks_rejected, 1);
        return false;
    }

    atomic_fetch_add(&pool->stats.tasks_submitted, 1);

    /* Track peak queue size */
    size_t current_size = safe_queue_size(&pool->queue);
    size_t peak = atomic_load(&pool->stats.peak_queue_size);
    while (current_size > peak) {
        if (atomic_compare_exchange_weak(&pool->stats.peak_queue_size,
                                          &peak, current_size)) {
            break;
        }
        peak = atomic_load(&pool->stats.peak_queue_size);
    }

    return true;
}

bool thread_pool_submit_task(thread_pool_t *pool, task_t *task) {
    if (pool == NULL || task == NULL) {
        return false;
    }

    if (atomic_load(&pool->state) != POOL_STATE_RUNNING) {
        atomic_fetch_add(&pool->stats.tasks_rejected, 1);
        return false;
    }

    if (!safe_queue_push(&pool->queue, task)) {
        atomic_fetch_add(&pool->stats.tasks_rejected, 1);
        return false;
    }

    atomic_fetch_add(&pool->stats.tasks_submitted, 1);
    return true;
}

/* ============================ */
/* Query & Statistics           */
/* ============================ */

size_t thread_pool_queue_size(thread_pool_t *pool) {
    if (pool == NULL) {
        return 0;
    }
    return safe_queue_size(&pool->queue);
}

size_t thread_pool_tasks_completed(thread_pool_t *pool) {
    if (pool == NULL) {
        return 0;
    }
    return (size_t)atomic_load(&pool->stats.tasks_completed);
}

bool thread_pool_is_running(thread_pool_t *pool) {
    if (pool == NULL) {
        return false;
    }
    return atomic_load(&pool->state) == POOL_STATE_RUNNING;
}

pool_state_t thread_pool_state(thread_pool_t *pool) {
    if (pool == NULL) {
        return POOL_STATE_SHUTDOWN;
    }
    return (pool_state_t)atomic_load(&pool->state);
}

void thread_pool_print_stats(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    printf("\n========== Thread Pool Statistics ==========\n");
    printf("  Pool Name:        %s\n", pool->config.name);
    printf("  Workers:          %zu\n", pool->worker_count);
    printf("  State:            %s\n",
           atomic_load(&pool->state) == POOL_STATE_RUNNING ? "RUNNING" :
           atomic_load(&pool->state) == POOL_STATE_SHUTTING_DOWN ? "SHUTTING_DOWN" :
           atomic_load(&pool->state) == POOL_STATE_SHUTDOWN ? "SHUTDOWN" : "INIT");
    printf("  Tasks Submitted:  %zu\n",
           (size_t)atomic_load(&pool->stats.tasks_submitted));
    printf("  Tasks Completed:  %zu\n",
           (size_t)atomic_load(&pool->stats.tasks_completed));
    printf("  Tasks Rejected:   %zu\n",
           (size_t)atomic_load(&pool->stats.tasks_rejected));
    printf("  Queue Size:       %zu\n", safe_queue_size(&pool->queue));
    printf("  Peak Queue Size:  %zu\n",
           (size_t)atomic_load(&pool->stats.peak_queue_size));
    printf("  Per-Worker Stats:\n");
    for (size_t i = 0; i < pool->worker_count; i++) {
        printf("    Worker %zu: %d tasks\n", i,
               atomic_load(&pool->workers[i].task_count));
    }
    printf("============================================\n\n");
}

void thread_pool_wait_all(thread_pool_t *pool) {
    if (pool == NULL) {
        return;
    }

    /* Simple approach: poll until queue is empty and all workers idle */
    while (1) {
        if (safe_queue_is_empty(&pool->queue)) {
            /* Check if all workers are idle */
            bool all_idle = true;
            for (size_t i = 0; i < pool->worker_count; i++) {
                if (atomic_load(&pool->workers[i].active)) {
                    all_idle = false;
                    break;
                }
            }
            if (all_idle) {
                break;
            }
        }
        /* Short sleep to avoid busy-waiting */
        usleep(1000); /* 1ms */
    }
}
