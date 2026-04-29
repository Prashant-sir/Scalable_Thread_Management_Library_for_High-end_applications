#ifndef LOGGER_H
#define LOGGER_H

/**
 * @file logger.h
 * @brief Simple logging utility for the thread library
 *
 * Provides timestamped log messages with log levels.
 * Thread-safe: uses mutex to prevent interleaved output.
 *
 * Log levels: DEBUG < INFO < WARN < ERROR
 * Set LOG_LEVEL at compile time: -DLOG_LEVEL=2
 */

#include <stdio.h>
#include <pthread.h>

#ifdef __cplusplus
extern "C" {
#endif

/* Log level constants */
#define LOG_LEVEL_DEBUG 0
#define LOG_LEVEL_INFO  1
#define LOG_LEVEL_WARN  2
#define LOG_LEVEL_ERROR 3
#define LOG_LEVEL_NONE  4

/* Default log level if not specified */
#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

/* Color codes for terminal output */
#define LOG_COLOR_RESET   "\033[0m"
#define LOG_COLOR_DEBUG   "\033[36m"  /* Cyan */
#define LOG_COLOR_INFO    "\033[32m"  /* Green */
#define LOG_COLOR_WARN    "\033[33m"  /* Yellow */
#define LOG_COLOR_ERROR   "\033[31m"  /* Red */

/**
 * @brief Log level names
 */
extern const char *log_level_names[];

/**
 * @brief Initialize the logging system
 */
void logger_init(void);

/**
 * @brief Clean up logging resources
 */
void logger_cleanup(void);

/**
 * @brief Set the minimum log level
 *
 * @param level One of LOG_LEVEL_* constants
 */
void logger_set_level(int level);

/**
 * @brief Core logging function (use macros instead)
 */
void logger_log(int level, const char *file, int line, const char *fmt, ...);

/* Convenience macros */
#define log_debug(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_DEBUG) logger_log(LOG_LEVEL_DEBUG, __FILE__, __LINE__, __VA_ARGS__); } while(0)

#define log_info(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_INFO) logger_log(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__); } while(0)

#define log_warn(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_WARN) logger_log(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__); } while(0)

#define log_error(...) \
    do { if (LOG_LEVEL <= LOG_LEVEL_ERROR) logger_log(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__); } while(0)

#ifdef __cplusplus
}
#endif

#endif /* LOGGER_H */
