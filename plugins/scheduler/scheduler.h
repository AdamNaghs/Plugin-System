#ifndef _SCHEDULER
#define _SCHEDULER

#include "../../include/plugin_api.h"

typedef void (*ScheduledFn)(CoreContext*, void*);

typedef struct {
    const char* name;
    float interval;
    float elapsed;
    ScheduledFn fn;
    void* user_data;
} ScheduledTask;

typedef struct {
    ScheduledTask* tasks;
    size_t count;
    size_t capacity;
} Scheduler;

void scheduler_register(const char* name, float interval, ScheduledFn fn, void* user_data);

#endif