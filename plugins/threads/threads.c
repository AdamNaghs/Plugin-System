#include "threads.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int core_thrd_create(thrd_t *thr, thrd_start_t func, void *arg)
{
    return thrd_create(thr, func, arg);
}

int core_thrd_join(thrd_t thr, int *res)
{
    return thrd_join(thr, res);
}

int core_thrd_detach(thrd_t thr)
{
    return thrd_detach(thr);
}

int core_thrd_sleep(const struct timespec *duration, struct timespec *remaining)
{
    return thrd_sleep(duration, remaining);
}

void core_thrd_exit(int res)
{
    thrd_exit(res);
}

void core_thrd_yield(void)
{
    thrd_yield();
}

#define MAX_THREADS 32
#define MAX_QUEUED_JOBS 64

typedef struct
{
    thrd_t handle;
    int active;
    int finished;
    void *user_data;
    int (*fn)(void *);
} ThreadJob;

typedef struct
{
    int (*fn)(void *);
    void *user_data;
} QueuedJob;

typedef struct
{
    QueuedJob *data;
    size_t head;
    size_t tail;
    size_t count;
    size_t capacity;
} JobQueue;

static JobQueue job_queue;

static ThreadJob jobs[MAX_THREADS];

static int thread_entry(void *arg)
{
    ThreadJob *job = (ThreadJob *)arg;
    job->fn(job->user_data);
    job->finished = 1;
    return 0;
}

void job_queue_init(JobQueue *q)
{
    q->capacity = 64; // initial capacity
    q->data = malloc(sizeof(QueuedJob) * q->capacity);
    q->head = q->tail = q->count = 0;
}

void job_queue_free(JobQueue *q)
{
    free(q->data);
    q->data = NULL;
    q->count = q->capacity = q->head = q->tail = 0;
}

void job_queue_push(JobQueue *q, int (*fn)(void *), void *user_data)
{
    if (q->count >= q->capacity)
    {
        size_t new_capacity = q->capacity * 2;
        QueuedJob *new_data = malloc(sizeof(QueuedJob) * new_capacity);

        // Copy circular queue into linear layout
        for (size_t i = 0; i < q->count; ++i)
            new_data[i] = q->data[(q->head + i) % q->capacity];

        free(q->data);
        q->data = new_data;
        q->head = 0;
        q->tail = q->count;
        q->capacity = new_capacity;
    }

    q->data[q->tail] = (QueuedJob){fn, user_data};
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
}

int job_queue_pop(JobQueue *q, QueuedJob *out)
{
    if (q->count == 0)
        return 0;

    *out = q->data[q->head];
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    return 1;
}

static int try_start_queued_job()
{
    if (job_queue.count == 0)
        return 0;

    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (!jobs[i].active)
        {
            QueuedJob qj;
            if (!job_queue_pop(&job_queue, &qj))
                return 0;

            jobs[i].active = 1;
            jobs[i].finished = 0;
            jobs[i].fn = qj.fn;
            jobs[i].user_data = qj.user_data;
            thrd_create(&jobs[i].handle, thread_entry, &jobs[i]);
            return 1;
        }
    }

    return 0;
}

void thread_spawn(int (*fn)(void *), void *user_data)
{
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (!jobs[i].active)
        {
            jobs[i].active = 1;
            jobs[i].finished = 0;
            jobs[i].fn = fn;
            jobs[i].user_data = user_data;
            thrd_create(&jobs[i].handle, thread_entry, &jobs[i]);
            return;
        }
    }

    // No thread slot available — queue it
    job_queue_push(&job_queue, fn, user_data);
}

int update(CoreContext *ctx)
{
    (void)ctx;

    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (jobs[i].active && jobs[i].finished)
        {
            thrd_join(jobs[i].handle, NULL);
            jobs[i].active = 0;
        }
    }

    // Try to start any queued jobs
    while (try_start_queued_job())
    {
    }

    return 0;
}

int init(CoreContext *ctx)
{
    memset(jobs, 0, sizeof(jobs));
    job_queue_init(&job_queue);

    // Job system
    CC_BIND(ctx, CC_THREAD_SPAWN, thread_spawn, sizeof(thread_spawn), false);

    // Raw thread functions
    CC_BIND(ctx, CC_THREAD_CREATE, core_thrd_create, sizeof(core_thrd_create), false);
    CC_BIND(ctx, CC_THREAD_JOIN, core_thrd_join, sizeof(core_thrd_join), false);
    CC_BIND(ctx, CC_THREAD_DETACH, core_thrd_detach, sizeof(core_thrd_detach), false);
    CC_BIND(ctx, CC_THREAD_SLEEP, core_thrd_sleep, sizeof(core_thrd_sleep), false);
    CC_BIND(ctx, CC_THREAD_EXIT, core_thrd_exit, sizeof(core_thrd_exit), false);
    CC_BIND(ctx, CC_THREAD_YIELD, core_thrd_yield, sizeof(core_thrd_yield), false);
    return 0;
}

int shutdown(CoreContext *ctx)
{
    (void)ctx;

    // Wait for all threads to finish
    for (int i = 0; i < MAX_THREADS; i++)
    {
        if (jobs[i].active)
        {
            thrd_join(jobs[i].handle, NULL);
            jobs[i].active = 0;
        }
    }

    job_queue_free(&job_queue);
    return 0;
}

static const char *deps[] = {NULL};
static const char *optional[] = {NULL};
static PluginMetadata meta = {"Threads", deps, optional};

PluginAPI Load()
{
    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta};
}

/*
int test_job(void* arg) {
    printf("Background: %s\n", (char*)arg);
    return 0;
}

int init(CoreContext* ctx) {
    void (*spawn)(int (*)(void*), void*) = CC_GET(ctx, "thread::spawn");

    for (int i = 0; i < 40; i++) {
        spawn(test_job, "Hello from job");
    }

    return 0;
}
*/