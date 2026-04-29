/**
 * @file logger.c
 * @brief Simple logging utility implementation
 */

#include "utils/logger.h"
#include <pthread.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

static pthread_mutex_t log_mutex = PTHREAD_MUTEX_INITIALIZER;
static int current_log_level = LOG_LEVEL;

const char *log_level_names[] = {
    "DEBUG",
    "INFO ",
    "WARN ",
    "ERROR"
};

void logger_init(void) {
    /* Mutex already statically initialized */
}

void logger_cleanup(void) {
    pthread_mutex_destroy(&log_mutex);
}

void logger_set_level(int level) {
    if (level >= LOG_LEVEL_DEBUG && level <= LOG_LEVEL_NONE) {
        current_log_level = level;
    }
}

void logger_log(int level, const char *file, int line, const char *fmt, ...) {
    if (level < current_log_level || level >= LOG_LEVEL_NONE) {
        return;
    }

    pthread_mutex_lock(&log_mutex);

    /* Get current timestamp */
    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", tm_info);

    /* Get thread ID */
    pthread_t tid = pthread_self();

    /* Select color */
    const char *color = LOG_COLOR_RESET;
    switch (level) {
        case LOG_LEVEL_DEBUG: color = LOG_COLOR_DEBUG; break;
        case LOG_LEVEL_INFO:  color = LOG_COLOR_INFO;  break;
        case LOG_LEVEL_WARN:  color = LOG_COLOR_WARN;  break;
        case LOG_LEVEL_ERROR: color = LOG_COLOR_ERROR; break;
    }

    /* Print header */
    fprintf(stderr, "%s[%s] [%s] [TID:%lu] %s:%d: ",
            color, timestamp, log_level_names[level],
            (unsigned long)tid, file, line);

    /* Print message */
    va_list args;
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);

    fprintf(stderr, "%s\n", LOG_COLOR_RESET);
    fflush(stderr);

    pthread_mutex_unlock(&log_mutex);
}
