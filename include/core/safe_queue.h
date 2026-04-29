#ifndef SAFE_QUEUE_H
#define SAFE_QUEUE_H

/**
 * @file safe_queue.h
 * @brief Thread-safe FIFO queue for task scheduling
 *
 * Uses pthread mutex and condition variable to ensure
 * safe concurrent access from multiple worker threads.
 */

#include "task.h"
#include <pthread.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Node in the linked-list queue
 */
typedef struct queue_node {
    task_t *task;               /* The task stored in this node */
    struct queue_node *next;    /* Pointer to next node */
} queue_node_t;

/**
 * @brief Thread-safe queue structure
 *
 * Uses a singly-linked list with head/tail pointers.
 * All operations are protected by a mutex.
 * Condition variable enables blocking wait on empty queue.
 */
typedef struct {
    queue_node_t *head;         /* Front of queue (for pop) */
    queue_node_t *tail;         /* Back of queue (for push) */
    size_t size;                /* Current number of items */
    size_t max_size;            /* Maximum capacity (0 = unlimited) */
    pthread_mutex_t mutex;      /* Protects all fields */
    pthread_cond_t not_empty;   /* Signaled when queue becomes non-empty */
    pthread_cond_t not_full;    /* Signaled when queue has space */
    bool shutdown;              /* true when queue is shutting down */
} safe_queue_t;

/**
 * @brief Initialize a new safe queue
 *
 * @param queue     The queue to initialize
 * @param max_size  Maximum capacity (0 for unlimited)
 * @return 0 on success, non-zero on failure
 */
int safe_queue_init(safe_queue_t *queue, size_t max_size);

/**
 * @brief Destroy queue and free all remaining nodes
 *
 * Any tasks still in the queue will be destroyed.
 *
 * @param queue The queue to destroy
 */
void safe_queue_destroy(safe_queue_t *queue);

/**
 * @brief Push a task onto the queue (blocking)
 *
 * Blocks if queue is at max capacity until space is available
 * or shutdown is initiated.
 *
 * @param queue The queue
 * @param task  The task to add
 * @return true on success, false if queue is shutting down
 */
bool safe_queue_push(safe_queue_t *queue, task_t *task);

/**
 * @brief Pop a task from the queue (blocking)
 *
 * Blocks until a task is available or shutdown is initiated.
 *
 * @param queue The queue
 * @return task_t* Pointer to task, or NULL if shutting down
 */
task_t *safe_queue_pop(safe_queue_t *queue);

/**
 * @brief Try to pop a task without blocking
 *
 * @param queue The queue
 * @param task  Output: pointer to store the popped task
 * @return true if a task was retrieved, false if empty
 */
bool safe_queue_try_pop(safe_queue_t *queue, task_t **task);

/**
 * @brief Get current queue size
 *
 * @param queue The queue
 * @return Number of tasks currently in queue
 */
size_t safe_queue_size(safe_queue_t *queue);

/**
 * @brief Check if queue is empty
 *
 * @param queue The queue
 * @return true if empty
 */
bool safe_queue_is_empty(safe_queue_t *queue);

/**
 * @brief Signal shutdown - wake all waiting threads
 *
 * After calling this, push() will reject new tasks and
 * pop() will return NULL to all waiting threads.
 *
 * @param queue The queue
 */
void safe_queue_shutdown(safe_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* SAFE_QUEUE_H */
