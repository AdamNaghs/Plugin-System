#include "signals.h"
#include "../include/plugin.h"
#include "../include/core_context.h"
#include "../include/memory_map.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

// HELPER METHODS

static MemoryMap signal_map;
static SignalQueueArray signal_queue;
static SignalID signal_id_counter = 1;

static void signal_queue_init(SignalQueueArray* q) {
    q->capacity = 16;
    q->count = 0;
    q->data = malloc(sizeof(QueuedSignal) * q->capacity);
}

static void signal_queue_push(SignalQueueArray* q, const char* name, void* sender, void* args) {
    if (q->count >= q->capacity) {
        q->capacity *= 2;
        q->data = realloc(q->data, q->capacity * sizeof(QueuedSignal));
    }
    q->data[q->count++] = (QueuedSignal){ name, sender, args };
}

static void signal_queue_clear(SignalQueueArray* q) {
    q->count = 0;
}

static void signal_queue_free(SignalQueueArray* q) {
    free(q->data);
    q->data = NULL;
    q->count = 0;
    q->capacity = 0;
}

SignalConnectionArray* get_or_create_signal_array(const char* name) {
    String key = STR((char*)name);
    SignalConnectionArray* arr = mm_get(&signal_map, key);
    if (!arr) {
        arr = malloc(sizeof(SignalConnectionArray));
        arr->count = 0;
        arr->capacity = 4;
        arr->data = malloc(sizeof(SignalConnection) * arr->capacity);
        mm_bind(&signal_map, key, arr, sizeof(*arr), true);
    }
    return arr;
}

SignalID signal_connection_array_push(SignalConnectionArray* arr, const char* name, SignalCallback cb, void* user_data) {
    if (arr->count == arr->capacity) {
        arr->capacity *= 2;
        arr->data = realloc(arr->data, arr->capacity * sizeof(SignalConnection));
    }
    SignalID id = signal_id_counter++;
    arr->data[arr->count++] = (SignalConnection){id, name, cb, user_data };
    return id;
}

void signal_flush(CoreContext* ctx) {
    for (size_t i = 0; i < signal_queue.count; i++) {
        QueuedSignal* s = &signal_queue.data[i];
        signal_emit(ctx, s->name, s->sender, s->args);
    }
    signal_queue_clear(&signal_queue);
}

// PUBLIC METHODS

SignalID signal_connect(const char* name, SignalCallback cb, void* user_data) {
    SignalConnectionArray* arr = get_or_create_signal_array(name);
    return signal_connection_array_push(arr, name, cb, user_data);
}

void signal_disconnect(SignalID id)
{
    for (size_t b = 0; b < signal_map.capacity; ++b)
    {
        MemoryBucket* bucket = &signal_map.buckets[b];
        for (size_t e = 0; e < bucket->count; ++e)
        {
            MemoryEntry* entry = &bucket->entries[e];
            if (!entry->data) continue;

            SignalConnectionArray* arr = (SignalConnectionArray*)entry->data;
            for (size_t i = 0; i < arr->count; ++i)
            {
                if (arr->data[i].id == id)
                {
                    // Remove connection by shifting the rest
                    memmove(&arr->data[i], &arr->data[i + 1], (arr->count - i - 1) * sizeof(SignalConnection));
                    arr->count--;
                    return;
                }
            }
        }
    }
}

void signal_emit(CoreContext* ctx, const char* name, void* sender, void* args) {
    SignalConnectionArray* arr = mm_get(&signal_map, STR((char*)name));
    if (!arr) return;

    for (size_t i = 0; i < arr->count; i++) {
        SignalConnection* conn = &arr->data[i];
        conn->callback(ctx, sender, args, conn->user_data);
    }
}

void signal_emit_deferred(const char* name, void* sender, void* args) {
    signal_queue_push(&signal_queue, name, sender, args);
}

// PLUGIN API

int init(CoreContext* ctx) {
    mm_init(&signal_map, 32, malloc, free, mm_hash_default);
    signal_queue_init(&signal_queue);

    CC_BIND(ctx, CC_SIGNAL_CONNECT, signal_connect, sizeof(signal_connect), false);
    CC_BIND(ctx, CC_SIGNAL_EMIT, signal_emit, sizeof(signal_emit), false);
    CC_BIND(ctx, CC_SIGNAL_DEFERRED, signal_emit_deferred, sizeof(signal_emit_deferred), false);

    return 0;
}

int update(CoreContext* ctx) {
    signal_flush(ctx);
    return 0;
}

int shutdown(CoreContext* ctx) {
    (void)ctx;
    signal_queue_free(&signal_queue);
    mm_free(&signal_map);
    return 0;
}

static const char* deps[] = { NULL };
static const char* optional[] = { NULL };
static PluginMetadata meta = { "Signals", deps, optional };

PluginAPI Load() {
    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta
    };
}

/*
#include "../include/plugin.h"
#include "../include/signals.h"  // assuming you expose your signal API

void on_boom(CoreContext* ctx, void* sender, void* args, void* user_data) {
    const char* message = (const char*)args;
    printf("💥 BOOM triggered: %s\n", message);
}

int init(CoreContext* ctx) {
    /// Get the connect function from the context
    void (*connect)(const char*, SignalCallback, void*) = CC_GET(ctx, "signal::connect");

    /// Connect the "boom" signal to your callback
    connect("boom", on_boom, NULL);

    return 0;
}

*/