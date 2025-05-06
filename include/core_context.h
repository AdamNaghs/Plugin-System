#ifndef _CORE_CONTEXT_H
#define _CORE_CONTEXT_H

#include "memory_map.h"

#define DEFAULT_MEMORY_BUCKETS 256

#define CTX_ALLOC(ctx,string,size) (ctx)->memory.alloc(&(ctx)->memory.map, LIT(string), size)
#define CTX_FREE(ctx,string) (ctx)->memory.free(&(ctx)->memory.map,LIT(string))
#define CTX_GET(ctx,string) (ctx)->memory.get(&(ctx)->memory.map,LIT(string))
typedef struct CoreContext
{
    int version;
    struct
    {
        MemoryMap map;
        void* (*alloc)(MemoryMap*, String, size_t); // allocates size_t bytes and returns it
        int (*free)(MemoryMap*, String); // frees data at const char* // map_remove
        void* (*get)(MemoryMap*, String);
    } memory;
    float delta_time;
    float fixed_delta_time;
} CoreContext;

void core_context_new(CoreContext* ctx, int version);

void core_context_free(CoreContext* ctx);

void core_context_update(CoreContext* ctx);

#endif /* _CORE_CONTEXT_H */