#include "threads.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


int core_mtx_init(mtx_t* mtx, int type) {
    return mtx_init(mtx, type);
}

void core_mtx_destroy(mtx_t* mtx) {
    mtx_destroy(mtx);
}

int core_mtx_lock(mtx_t* mtx) {
    return mtx_lock(mtx);
}

int core_mtx_trylock(mtx_t* mtx) {
    return mtx_trylock(mtx);
}

int core_mtx_unlock(mtx_t* mtx) {
    return mtx_unlock(mtx);
}

int core_mtx_timedlock(mtx_t* mtx, const struct timespec* ts) {
    return mtx_timedlock(mtx, ts);
}

int core_thrd_create(thrd_t* thr, thrd_start_t func, void* arg) {
    return thrd_create(thr, func, arg);
}

int core_thrd_join(thrd_t thr, int* res) {
    return thrd_join(thr, res);
}

int core_thrd_detach(thrd_t thr) {
    return thrd_detach(thr);
}

int core_thrd_sleep(const struct timespec* duration, struct timespec* remaining) {
    return thrd_sleep(duration, remaining);
}

void core_thrd_exit(int res) {
    thrd_exit(res);
}

void core_thrd_yield(void) {
    thrd_yield();
}


#define MAX_THREADS 32
#define MAX_QUEUED_JOBS 64

typedef struct {
    thrd_t handle;
    int active;
    int finished;
    void* user_data;
    int (*fn)(void*);
} ThreadJob;

typedef struct {
    int (*fn)(void*);
    void* user_data;
} QueuedJob;

static ThreadJob jobs[MAX_THREADS];
static QueuedJob job_queue[MAX_QUEUED_JOBS];
static int queue_head = 0;
static int queue_tail = 0;
static int queue_count = 0;

static int thread_entry(void* arg) {
    ThreadJob* job = (ThreadJob*)arg;
    job->fn(job->user_data);
    job->finished = 1;
    return 0;
}

static void enqueue_job(int (*fn)(void*), void* user_data) {
    if (queue_count >= MAX_QUEUED_JOBS) {
        printf("Thread queue overflow. Running on main thread.\n");
        (*fn)(user_data);
        return;
    }
    job_queue[queue_tail] = (QueuedJob){ fn, user_data };
    queue_tail = (queue_tail + 1) % MAX_QUEUED_JOBS;
    queue_count++;
}

static int try_start_queued_job() {
    if (queue_count == 0) return 0;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (!jobs[i].active) {
            QueuedJob qj = job_queue[queue_head];
            queue_head = (queue_head + 1) % MAX_QUEUED_JOBS;
            queue_count--;

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

void thread_spawn(int (*fn)(void*), void* user_data) {
    for (int i = 0; i < MAX_THREADS; i++) {
        if (!jobs[i].active) {
            jobs[i].active = 1;
            jobs[i].finished = 0;
            jobs[i].fn = fn;
            jobs[i].user_data = user_data;
            thrd_create(&jobs[i].handle, thread_entry, &jobs[i]);
            return;
        }
    }

    // No thread slot available — queue it
    enqueue_job(fn, user_data);
}

int update(CoreContext* ctx) {
    (void)ctx;

    for (int i = 0; i < MAX_THREADS; i++) {
        if (jobs[i].active && jobs[i].finished) {
            thrd_join(jobs[i].handle, NULL);
            jobs[i].active = 0;
        }
    }

    // Try to start any queued jobs
    while (try_start_queued_job()) {}

    return 0;
}

int init(CoreContext* ctx) {
    memset(jobs, 0, sizeof(jobs));
    queue_head = queue_tail = queue_count = 0;
    CC_BIND(ctx, "thread::spawn", thread_spawn, sizeof(thread_spawn), false);

    CC_BIND(ctx, "thread::mtx_init", core_mtx_init, sizeof(core_mtx_init), false);
    CC_BIND(ctx, "thread::mtx_destroy", core_mtx_destroy, sizeof(core_mtx_destroy), false);
    CC_BIND(ctx, "thread::mtx_lock", core_mtx_lock, sizeof(core_mtx_lock), false);
    CC_BIND(ctx, "thread::mtx_trylock", core_mtx_trylock, sizeof(core_mtx_trylock), false);
    CC_BIND(ctx, "thread::mtx_unlock", core_mtx_unlock, sizeof(core_mtx_unlock), false);
    CC_BIND(ctx, "thread::mtx_timedlock", core_mtx_timedlock, sizeof(core_mtx_timedlock), false);

    CC_BIND(ctx, "thread::raw::create", core_thrd_create, sizeof(thrd_create), false);
    CC_BIND(ctx, "thread::raw::join", core_thrd_join, sizeof(thrd_join), false);
    CC_BIND(ctx, "thread::raw::detach", core_thrd_detach, sizeof(thrd_detach), false);
    CC_BIND(ctx, "thread::raw::sleep", core_thrd_sleep, sizeof(thrd_sleep), false);
    CC_BIND(ctx, "thread::raw::exit", core_thrd_exit, sizeof(thrd_exit), false);
    CC_BIND(ctx, "thread::raw::yield", core_thrd_yield, sizeof(thrd_yield), false);

    return 0;
}

int shutdown(CoreContext* ctx) {
    (void)ctx;

    // Wait for all threads to finish
    for (int i = 0; i < MAX_THREADS; i++) {
        if (jobs[i].active) {
            thrd_join(jobs[i].handle, NULL);
            jobs[i].active = 0;
        }
    }

    queue_head = queue_tail = queue_count = 0;
    return 0;
}

static const char* deps[] = { NULL };
static PluginMetadata meta = { "Threads", deps };

PluginAPI Load() {
    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta
    };
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