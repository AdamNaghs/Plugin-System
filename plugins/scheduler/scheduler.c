#include "scheduler.h"
#include <stdlib.h>

#define DEFAULT_CAPACITY 16

static Scheduler* scheduler = NULL; 

typedef void (*SchedulerRegisterFn)(const char*, float, ScheduledFn, void*);


int init(CoreContext* ctx)
{
    scheduler = ctx->memory.alloc(&ctx->memory.map,LIT("SCHEDULER"),sizeof(Scheduler));
    SchedulerRegisterFn* fn = CC_ALLOC(ctx,"SCHEDULER::Register",sizeof(scheduler_register));
    *fn = scheduler_register;
    scheduler->tasks = malloc(DEFAULT_CAPACITY * sizeof(ScheduledTask));
    scheduler->capacity = DEFAULT_CAPACITY;
    scheduler->count = 0;
    return 0;
};

int shutdown(CoreContext* ctx)
{
    (void)ctx; /* suppress unused variable */
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

void scheduler_register(const char* name, float interval, ScheduledFn fn, void* user_data)
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

static const char* deps[] = {NULL};

static PluginMetadata meta = {"Scheduler", deps};

PluginAPI Load()
{
    return (PluginAPI){init,update,shutdown,&meta};
}