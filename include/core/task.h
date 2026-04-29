#ifndef TASK_H
#define TASK_H

/**
 * @file task.h
 * @brief Task encapsulation for the thread pool
 *
 * A Task wraps a function pointer and its argument,
 * allowing generic work to be submitted to the thread pool.
 */

#ifdef __cplusplus
extern "C" {
#endif

/* Function pointer type for task functions */
typedef void (*task_func_t)(void *arg);

/**
 * @brief Task structure
 *
 * Encapsulates a unit of work: a function to execute
 * and its associated argument.
 */
typedef struct {
    task_func_t func;  /* Pointer to the function to execute */
    void *arg;         /* Argument passed to the function */
} task_t;

/**
 * @brief Create a new task
 *
 * @param func The function to execute (cannot be NULL)
 * @param arg  Argument to pass to the function (can be NULL)
 * @return task_t* Pointer to allocated task, or NULL on failure
 */
task_t *task_create(task_func_t func, void *arg);

/**
 * @brief Destroy a task and free its memory
 *
 * @param task The task to destroy
 */
void task_destroy(task_t *task);

/**
 * @brief Execute a task
 *
 * Calls the task's function with its argument.
 *
 * @param task The task to execute
 */
void task_execute(task_t *task);

#ifdef __cplusplus
}
#endif

#endif /* TASK_H */
