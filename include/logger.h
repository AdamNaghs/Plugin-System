#ifndef _CORE_LOGGER_H
#define _CORE_LOGGER_H

#include <stdio.h>

typedef enum LogLevel
{
    LL_INFO,
    LL_WARN,
    LL_ERROR,
    LL_DEBUG,
} LogLevel;

void logger_init();

void logger_shutdown();

void logger(enum LogLevel level, const char* fmt, ...);

#endif /*_CORE_LOGGER_H*/