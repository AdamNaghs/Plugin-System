#ifndef _CORE_CONTEXT_H
#define _CORE_CONTEXT_H

#include "memory_ctx.h"

#define DEFAULT_MEMORY_BUCKETS 256

typedef struct CoreContext
{
    int version;
    struct
    {
        Map* map;
        void* (*alloc)(Map*,const char*, size_t); // allocates size_t bytes and returns it
        int (*free)(Map*,const char*); // frees data at const char* // map_remove
        void* (*get)(Map*, const char*);
        void (*set)(Map*, const char*, void*, size_t); // use MEM_CTX_SET if you're confused
    } memory;
    float delta_time;
    float fixed_delta_time;
} CoreContext;

void core_context_new(CoreContext* ctx, int version);

void core_context_free(CoreContext* ctx);

void core_context_update(CoreContext* ctx);

#endif /* _CORE_CONTEXT_H */