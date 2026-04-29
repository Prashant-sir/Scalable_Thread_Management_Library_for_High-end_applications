/**
 * @file task.c
 * @brief Task implementation
 */

#include "core/task.h"
#include <stdlib.h>

task_t *task_create(task_func_t func, void *arg) {
    if (func == NULL) {
        return NULL;
    }

    task_t *task = (task_t *)malloc(sizeof(task_t));
    if (task == NULL) {
        return NULL;
    }

    task->func = func;
    task->arg = arg;
    return task;
}

void task_destroy(task_t *task) {
    if (task != NULL) {
        free(task);
    }
}

void task_execute(task_t *task) {
    if (task != NULL && task->func != NULL) {
        task->func(task->arg);
    }
}
