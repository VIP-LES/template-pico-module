// src/log.h

#pragma once

#include "pico/time.h"
#include <stdio.h>

// Define log levels
typedef enum {
    LOG_LEVEL_INFO,
    LOG_LEVEL_WARN,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_FATAL
} LogLevel;

/**
 * @brief The core logging function.
 * * @param level The log level (e.g., LOG_LEVEL_INFO).
 * @param file The source file name where the log originates.
 * @param line The line number in the source file.
 * @param fmt The format string (like printf).
 * @param ... Variadic arguments for the format string.
 */
void log_message(LogLevel level, const char* file, int line, const char* fmt, ...);

// Define convenient logging macros
#define LOG_INFO(...) log_message(LOG_LEVEL_INFO, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_WARN(...) log_message(LOG_LEVEL_WARN, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_ERROR(...) log_message(LOG_LEVEL_ERROR, __FILE__, __LINE__, __VA_ARGS__)
#define LOG_FATAL(...) log_message(LOG_LEVEL_FATAL, __FILE__, __LINE__, __VA_ARGS__)