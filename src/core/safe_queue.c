/**
 * @file safe_queue.c
 * @brief Thread-safe queue implementation using pthread synchronization
 *
 * Uses a singly-linked list protected by a mutex.
 * Condition variables enable efficient blocking operations.
 */

#include "core/safe_queue.h"
#include <stdlib.h>

int safe_queue_init(safe_queue_t *queue, size_t max_size) {
    if (queue == NULL) {
        return -1;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;
    queue->max_size = max_size;
    queue->shutdown = false;

    if (pthread_mutex_init(&queue->mutex, NULL) != 0) {
        return -1;
    }

    if (pthread_cond_init(&queue->not_empty, NULL) != 0) {
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }

    if (pthread_cond_init(&queue->not_full, NULL) != 0) {
        pthread_cond_destroy(&queue->not_empty);
        pthread_mutex_destroy(&queue->mutex);
        return -1;
    }

    return 0;
}

void safe_queue_destroy(safe_queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    /* Drain any remaining tasks */
    queue_node_t *current = queue->head;
    while (current != NULL) {
        queue_node_t *next = current->next;
        if (current->task != NULL) {
            task_destroy(current->task);
        }
        free(current);
        current = next;
    }

    queue->head = NULL;
    queue->tail = NULL;
    queue->size = 0;

    pthread_cond_destroy(&queue->not_empty);
    pthread_cond_destroy(&queue->not_full);
    pthread_mutex_destroy(&queue->mutex);
}

bool safe_queue_push(safe_queue_t *queue, task_t *task) {
    if (queue == NULL || task == NULL) {
        return false;
    }

    pthread_mutex_lock(&queue->mutex);

    /* Reject new tasks if shutting down */
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }

    /* Wait if queue is at capacity */
    while (queue->max_size > 0 && queue->size >= queue->max_size && !queue->shutdown) {
        pthread_cond_wait(&queue->not_full, &queue->mutex);
    }

    /* Check again after waking */
    if (queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }

    /* Create new node */
    queue_node_t *node = (queue_node_t *)malloc(sizeof(queue_node_t));
    if (node == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }

    node->task = task;
    node->next = NULL;

    /* Append to tail */
    if (queue->tail != NULL) {
        queue->tail->next = node;
    } else {
        queue->head = node;
    }
    queue->tail = node;
    queue->size++;

    /* Signal waiting consumers */
    pthread_cond_signal(&queue->not_empty);
    pthread_mutex_unlock(&queue->mutex);

    return true;
}

task_t *safe_queue_pop(safe_queue_t *queue) {
    if (queue == NULL) {
        return NULL;
    }

    pthread_mutex_lock(&queue->mutex);

    /* Wait until queue has items or shutdown */
    while (queue->head == NULL && !queue->shutdown) {
        pthread_cond_wait(&queue->not_empty, &queue->mutex);
    }

    /* Return NULL if shutting down and queue empty */
    if (queue->shutdown && queue->head == NULL) {
        pthread_mutex_unlock(&queue->mutex);
        return NULL;
    }

    /* Remove from head */
    queue_node_t *node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;

    /* Signal waiting producers */
    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    task_t *task = node->task;
    free(node);
    return task;
}

bool safe_queue_try_pop(safe_queue_t *queue, task_t **task) {
    if (queue == NULL || task == NULL) {
        return false;
    }

    pthread_mutex_lock(&queue->mutex);

    if (queue->head == NULL || queue->shutdown) {
        pthread_mutex_unlock(&queue->mutex);
        return false;
    }

    /* Remove from head */
    queue_node_t *node = queue->head;
    queue->head = node->next;
    if (queue->head == NULL) {
        queue->tail = NULL;
    }
    queue->size--;

    pthread_cond_signal(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);

    *task = node->task;
    free(node);
    return true;
}

size_t safe_queue_size(safe_queue_t *queue) {
    if (queue == NULL) {
        return 0;
    }

    pthread_mutex_lock(&queue->mutex);
    size_t size = queue->size;
    pthread_mutex_unlock(&queue->mutex);
    return size;
}

bool safe_queue_is_empty(safe_queue_t *queue) {
    return safe_queue_size(queue) == 0;
}

void safe_queue_shutdown(safe_queue_t *queue) {
    if (queue == NULL) {
        return;
    }

    pthread_mutex_lock(&queue->mutex);
    queue->shutdown = true;
    /* Wake all waiting threads */
    pthread_cond_broadcast(&queue->not_empty);
    pthread_cond_broadcast(&queue->not_full);
    pthread_mutex_unlock(&queue->mutex);
}
