#include "scheduler.h"
#include <stdlib.h>

#define DEFAULT_CAPACITY 16

static Scheduler* scheduler = NULL; 

int init(CoreContext* ctx)
{
    scheduler = ctx->memory.alloc(&ctx->memory.map,LIT("SCHEDULER"),sizeof(Scheduler));
    scheduler->tasks = malloc(DEFAULT_CAPACITY * sizeof(ScheduledTask));
    scheduler->capacity = DEFAULT_CAPACITY;
    scheduler->count = 0;
    return 0;
};

int shutdown(CoreContext* ctx)
{
    free(scheduler->tasks);
    return 0;
}

int update(CoreContext* ctx) {
    for (size_t i = 0; i < scheduler->count; i++) {
        ScheduledTask* task = &scheduler->tasks[i];
        task->elapsed += ctx->delta_time;
        if (task->elapsed >= task->interval) {
            task->fn(ctx, task->user_data);
            task->elapsed = 0.0f;
        }
    }
    return 0;
}

void scheduler_register(CoreContext* ctx, const char* name, float interval, ScheduledFn fn, void* user_data)
{
    if (!scheduler || !fn || interval <= 0.0f) return;

    if (scheduler->count >= scheduler->capacity) {
        size_t new_capacity = scheduler->capacity * 2;
        scheduler->tasks = realloc(scheduler->tasks, new_capacity * sizeof(ScheduledTask));
        scheduler->capacity = new_capacity;
    }

    ScheduledTask* task = &scheduler->tasks[scheduler->count++];
    task->name = name; // optional if you're using it
    task->interval = interval;
    task->elapsed = 0.0f;
    task->fn = fn;
    task->user_data = user_data;
}

PluginAPI Load()
{
    return (PluginAPI){init,update,NULL,shutdown};
}