#include "../include/logger.h"

#ifdef _WIN32
#include <direct.h>
#define mkdir(path, mode) _mkdir(path)
#else
#include <sys/stat.h>
#include <sys/types.h>
#endif

#include <errno.h>
#include <time.h>
#include <stdarg.h>
#include <string.h>

static FILE* log_fp = NULL;

// Create logs directory if it doesn't exist
int ensure_logs_dir() {
    return mkdir("logs", 0755); // safe to ignore EEXIST
}

// Format: logs/YYYY-MM-DD_HH-MM-SS.log
void make_log_filename(char* out, size_t size) {
    time_t now = time(NULL);
    struct tm* t = localtime(&now);
    snprintf(out, size, "logs/%04d-%02d-%02d_%02d-%02d-%02d.log",
             t->tm_year + 1900, t->tm_mon + 1, t->tm_mday,
             t->tm_hour, t->tm_min, t->tm_sec);
}

// Open log file
void init_file(const char* path) {
    log_fp = fopen(path, "w");
    if (!log_fp) {
        fprintf(stderr, "[LOG] Failed to open log file: %s\n", path);
    }
}

// Close log file
void log_close() {
    if (log_fp) {
        fclose(log_fp);
        log_fp = NULL;
    }
}

// Core logging function
void logger(LogLevel level, const char* fmt, ...) {
    const char* prefix = "";
    switch (level) {
        case LL_INFO:  prefix = "[INFO]";  break;
        case LL_WARN:  prefix = "[WARN]";  break;
        case LL_ERROR: prefix = "[ERROR]"; break;
        case LL_DEBUG: prefix = "[DEBUG]"; break;
    }

    // Get current time
    time_t now = time(NULL);
    struct tm* t = localtime(&now);

    char time_buf[20];
    strftime(time_buf, sizeof(time_buf), "%Y-%m-%d %H:%M:%S", t);

    va_list args;
    va_start(args, fmt);

    // Console output
    fprintf(stderr, "[%s] %s ", time_buf, prefix);
    vfprintf(stderr, fmt, args);
    fprintf(stderr, "\n");

    // File output
    if (log_fp) {
        va_list copy;
        va_copy(copy, args);
        fprintf(log_fp, "[%s] %s ", time_buf, prefix);
        vfprintf(log_fp, fmt, copy);
        fprintf(log_fp, "\n");
        fflush(log_fp);
        va_end(copy);
    }

    va_end(args);
}


// Initialize logger: create directory, open file
void logger_init() {
    ensure_logs_dir();
    char path[256];
    make_log_filename(path, sizeof(path));
    init_file(path);
}

// Clean shutdown
void logger_shutdown() {
    log_close();
}
