// src/log.c

#include "log.h"
#include "pico/stdlib.h"
#include <stdarg.h>

// Array of strings for log level names
static const char* level_strings[] = {
    "INFO", "WARN", "ERROR", "FATAL"
};

void log_message(LogLevel level, const char* file, int line, const char* fmt, ...)
{
    // Get current time in milliseconds
    uint32_t timestamp_ms = to_ms_since_boot(get_absolute_time());

    // Print the log prefix
    printf("[%u] [%s] ", timestamp_ms, level_strings[level]);

    // Print the actual log message using variadic arguments
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);

    // Print file and line for debug-ability, then a newline
    printf(" (%s:%d)\n", file, line);

    // If the level is FATAL, halt the program
    if (level == LOG_LEVEL_FATAL) {
        printf("Halting due to FATAL error.\n");
        while (true) {
            tight_loop_contents();
        }
    }
}