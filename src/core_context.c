#include "../include/core_context.h"
#include "../include/dt.h"


void core_context_new(CoreContext* ctx, int version)
{
    ctx->version = version;
    mm_init(&ctx->memory.map,DEFAULT_MEMORY_BUCKETS,malloc,free,mm_hash_default);
    ctx->memory.alloc = mm_alloc;
    ctx->memory.free = mm_remove;
    ctx->memory.get = mm_get;
    ctx->memory.bind = mm_bind;
}

void core_context_free(CoreContext* ctx)
{
    mm_free(&ctx->memory.map);
    ctx->memory.alloc = NULL;
    ctx->memory.free = NULL;
    ctx->memory.get = NULL;
}

void core_context_update(CoreContext* ctx)
{
    dt_update();
    ctx->delta_time = get_dt();
}