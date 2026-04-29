#ifndef THREAD_POOL_H
#define THREAD_POOL_H

/**
 * @file thread_pool.h
 * @brief High-performance thread pool for concurrent task execution
 *
 * Manages a fixed pool of worker threads that continuously
 * fetch and execute tasks from a shared thread-safe queue.
 *
 * Usage:
 *   thread_pool_t pool;
 *   thread_pool_init(&pool, 4);          // 4 worker threads
 *   thread_pool_submit(&pool, my_func, arg);
 *   thread_pool_shutdown(&pool);
 */

#include "safe_queue.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Forward declaration */
struct thread_pool;

/**
 * @brief Worker thread state
 */
typedef struct worker {
    pthread_t thread;           /* OS thread handle */
    struct thread_pool *pool;   /* Parent pool reference */
    atomic_int task_count;      /* Tasks executed by this worker */
    atomic_bool active;         /* Currently processing a task */
    int worker_id;              /* Unique identifier */
} worker_t;

/**
 * @brief Thread pool configuration
 */
typedef struct {
    size_t thread_count;        /* Number of worker threads */
    size_t queue_capacity;      /* Max queue size (0 = unlimited) */
    const char *name;           /* Optional pool name for logging */
} pool_config_t;

/**
 * @brief Thread pool statistics
 */
typedef struct {
    atomic_size_t tasks_submitted;   /* Total tasks submitted */
    atomic_size_t tasks_completed;   /* Total tasks completed */
    atomic_size_t tasks_rejected;    /* Tasks rejected (queue full) */
    atomic_size_t peak_queue_size;   /* Maximum queue size reached */
} pool_stats_t;

/**
 * @brief Thread pool state
 */
typedef enum {
    POOL_STATE_INIT,        /* Pool created but not started */
    POOL_STATE_RUNNING,     /* Accepting and processing tasks */
    POOL_STATE_SHUTTING_DOWN, /* Shutdown requested, draining queue */
    POOL_STATE_SHUTDOWN     /* All workers stopped */
} pool_state_t;

/**
 * @brief Main thread pool structure
 */
typedef struct thread_pool {
    worker_t *workers;          /* Array of worker threads */
    size_t worker_count;        /* Number of workers */
    safe_queue_t queue;         /* Shared task queue */
    pool_config_t config;       /* Pool configuration */
    pool_stats_t stats;         /* Runtime statistics */
    atomic_int state;           /* Current pool state */
    pthread_mutex_t pool_mutex; /* Protects state changes */
    pthread_cond_t pool_cond;   /* For shutdown synchronization */
} thread_pool_t;

/* ============================ */
/* Configuration Helpers        */
/* ============================ */

/**
 * @brief Get default pool configuration
 *
 * Defaults: thread_count = CPU cores, queue_capacity = unlimited
 *
 * @return Default configuration
 */
pool_config_t pool_config_default(void);

/**
 * @brief Create configuration with specified thread count
 *
 * @param thread_count Number of worker threads
 * @return Configuration struct
 */
pool_config_t pool_config_with_threads(size_t thread_count);

/* ============================ */
/* Lifecycle Management         */
/* ============================ */

/**
 * @brief Initialize thread pool with configuration
 *
 * Creates worker threads and starts them waiting for tasks.
 *
 * @param pool   The pool to initialize
 * @param config Pool configuration
 * @return 0 on success, non-zero on failure
 */
int thread_pool_init(thread_pool_t *pool, pool_config_t config);

/**
 * @brief Initialize thread pool with default settings
 *
 * Convenience function using automatic thread count detection.
 *
 * @param pool The pool to initialize
 * @return 0 on success, non-zero on failure
 */
int thread_pool_init_default(thread_pool_t *pool);

/**
 * @brief Shutdown the thread pool gracefully
 *
 * Signals workers to stop after completing current tasks.
 * Waits for all workers to finish.
 *
 * @param pool The pool to shutdown
 */
void thread_pool_shutdown(thread_pool_t *pool);

/**
 * @brief Shutdown and destroy all resources
 *
 * Convenience: shutdown + free all allocated memory.
 *
 * @param pool The pool to destroy
 */
void thread_pool_destroy(thread_pool_t *pool);

/* ============================ */
/* Task Submission              */
/* ============================ */

/**
 * @brief Submit a task to the pool
 *
 * Adds a task to the queue. A worker thread will pick it up.
 *
 * @param pool The pool
 * @param func Function to execute
 * @param arg  Argument for the function (can be NULL)
 * @return true on success, false if pool is shutting down or queue full
 */
bool thread_pool_submit(thread_pool_t *pool, task_func_t func, void *arg);

/**
 * @brief Submit a pre-created task
 *
 * @param pool The pool
 * @param task The task to submit (ownership transferred to pool)
 * @return true on success
 */
bool thread_pool_submit_task(thread_pool_t *pool, task_t *task);

/* ============================ */
/* Query & Statistics           */
/* ============================ */

/**
 * @brief Get current number of queued tasks
 *
 * @param pool The pool
 * @return Number of tasks waiting in queue
 */
size_t thread_pool_queue_size(thread_pool_t *pool);

/**
 * @brief Get total number of completed tasks
 *
 * @param pool The pool
 * @return Number of tasks that have finished executing
 */
size_t thread_pool_tasks_completed(thread_pool_t *pool);

/**
 * @brief Check if pool is running
 *
 * @param pool The pool
 * @return true if pool is accepting tasks
 */
bool thread_pool_is_running(thread_pool_t *pool);

/**
 * @brief Get current pool state
 *
 * @param pool The pool
 * @return Current state enum value
 */
pool_state_t thread_pool_state(thread_pool_t *pool);

/**
 * @brief Print pool statistics to stdout
 *
 * @param pool The pool
 */
void thread_pool_print_stats(thread_pool_t *pool);

/**
 * @brief Wait for all submitted tasks to complete
 *
 * Blocks until the queue is empty and all workers are idle.
 *
 * @param pool The pool
 */
void thread_pool_wait_all(thread_pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* THREAD_POOL_H */
