#include "../include/core_context.h"
#include "../include/dt.h"

void* get(Map* map, const char* key)
{
   return map_get(map,(const void*)key);
}

int remove(Map* map, const char* key)
{
    return map_remove(map,(const void*)key);
}

void core_context_new(CoreContext* ctx, int version)
{
    ctx->version = version;
    ctx->memory.map = map_new(get_mem_ctx_map_type(),DEFAULT_MEMORY_BUCKETS);
    ctx->memory.alloc = mem_ctx_alloc;
    ctx->memory.free = remove;
    ctx->memory.get = get;
    ctx->memory.set = mem_ctx_set;
}

void core_context_free(CoreContext* ctx)
{
    map_free(ctx->memory.map);
    ctx->memory.map = NULL;
    ctx->memory.alloc = NULL;
    ctx->memory.free = NULL;
    ctx->memory.get = NULL;
    ctx->memory.set = NULL;
}

void core_context_update(CoreContext* ctx)
{
    dt_update();
    ctx->delta_time = get_dt();
}