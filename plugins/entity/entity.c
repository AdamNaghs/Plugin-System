#include "../include/plugin_api.h"
#include "entity.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define INITIAL_ENTITY_CAPACITY 64
#define INITIAL_ENTITY_MAP_CAPACITY 128

typedef struct
{
    EntityID id;
    size_t index;
} EntityMapEntry;

static Entity *entities = NULL;
static size_t entity_count = 0;
static size_t entity_capacity = 0;

static EntityMapEntry *entity_map = NULL;
static size_t entity_map_count = 0;
static size_t entity_map_capacity = 0;

static EntityID next_entity_id = 1;

#define ENTITY_MAP_EMPTY_ID ENTITY_INVALID_ID

// --- HELPER FUNCTIONS ---

static size_t entity_hash(EntityID id)
{
    return (size_t)(id ^ (id >> 32)); // simple 64-bit xor fold
}

static void ensure_entity_capacity(size_t required)
{
    if (required <= entity_capacity)
        return;

    size_t new_capacity = entity_capacity ? entity_capacity : INITIAL_ENTITY_CAPACITY;
    while (new_capacity < required)
    {
        new_capacity *= 2;
    }

    Entity *new_entities = realloc(entities, new_capacity * sizeof(Entity));
    if (!new_entities)
    {
        fprintf(stderr, "Failed to realloc entity array!\n");
        exit(1);
    }
    entities = new_entities;
    entity_capacity = new_capacity;
}

static void entity_map_clear(EntityMapEntry *map, size_t cap)
{
    for (size_t i = 0; i < cap; ++i)
    {
        map[i].id = ENTITY_MAP_EMPTY_ID;
        map[i].index = 0;
    }
}

static void ensure_entity_map_capacity(size_t required)
{
    if (required <= entity_map_capacity)
        return;

    size_t new_capacity = entity_map_capacity ? entity_map_capacity : INITIAL_ENTITY_MAP_CAPACITY;
    while (new_capacity < required)
    {
        new_capacity *= 2;
    }

    EntityMapEntry *new_map = malloc(sizeof(EntityMapEntry) * new_capacity);
    if (!new_map)
    {
        fprintf(stderr, "Failed to realloc entity map!\n");
        exit(1);
    }
    entity_map_clear(new_map, new_capacity);

    // Rehash old entries if existing
    if (entity_map)
    {
        for (size_t i = 0; i < entity_map_capacity; ++i)
        {
            if (entity_map[i].id != ENTITY_MAP_EMPTY_ID)
            {
                // Re-insert into new map
                size_t mask = new_capacity - 1;
                size_t idx = entity_hash(entity_map[i].id) & mask;
                while (new_map[idx].id != ENTITY_MAP_EMPTY_ID)
                {
                    idx = (idx + 1) & mask;
                }
                new_map[idx] = entity_map[i];
            }
        }
        free(entity_map);
    }

    entity_map = new_map;
    entity_map_capacity = new_capacity;
}

static void entity_map_insert(EntityID id, size_t index)
{
    ensure_entity_map_capacity(entity_map_count + 1);

    size_t mask = entity_map_capacity - 1;
    size_t idx = entity_hash(id) & mask;

    while (entity_map[idx].id != ENTITY_MAP_EMPTY_ID)
    {
        idx = (idx + 1) & mask;
    }

    entity_map[idx].id = id;
    entity_map[idx].index = index;
    entity_map_count++;
}

static ssize_t entity_map_find(EntityID id)
{
    if (!entity_map_capacity)
        return -1;

    size_t mask = entity_map_capacity - 1;
    size_t idx = entity_hash(id) & mask;

    for (size_t i = 0; i < entity_map_capacity; ++i)
    {
        if (entity_map[idx].id == ENTITY_MAP_EMPTY_ID)
        {
            return -1; // Not found
        }
        if (entity_map[idx].id == id)
        {
            return (ssize_t)idx;
        }
        idx = (idx + 1) & mask;
    }
    return -1; // Should not happen
}

static void entity_map_remove(EntityID id)
{
    ssize_t found_idx = entity_map_find(id);
    if (found_idx < 0)
        return;

    size_t idx = (size_t)found_idx;
    entity_map[idx].id = ENTITY_MAP_EMPTY_ID;
    entity_map[idx].index = 0;
    entity_map_count--;

    size_t mask = entity_map_capacity - 1;
    idx = (idx + 1) & mask;

    while (entity_map[idx].id != ENTITY_MAP_EMPTY_ID)
    {
        EntityID rehash_id = entity_map[idx].id;
        size_t rehash_index = entity_map[idx].index;

        entity_map[idx].id = ENTITY_MAP_EMPTY_ID;
        entity_map[idx].index = 0;
        entity_map_count--;

        // Reinsert the entry
        entity_map_insert(rehash_id, rehash_index);

        idx = (idx + 1) & mask;
    }
}

// --- ENTITY FUNCTIONS ---

static Entity *get_free_entity_slot(void)
{
    ensure_entity_capacity(entity_count + 1);
    return &entities[entity_count++];
}

static void flush_destroyed_entities(void)
{
    size_t dst = 0;
    for (size_t src = 0; src < entity_count; ++src)
    {
        Entity *e = &entities[src];
        if (e->meta.state == ES_QUEUED_FREE)
        {
            if (e->type.methods.shutdown)
            {
                e->type.methods.shutdown(e);
            }
            free(e->user_data);
            e->meta.state = ES_FREED;
            entity_map_remove(e->meta.id);
            continue;
        }
        if (dst != src)
        {
            entities[dst] = entities[src];
            // Update the entity map with new index
            ssize_t map_idx = entity_map_find(entities[dst].meta.id);
            if (map_idx >= 0)
            {
                entity_map[map_idx].index = dst;
            }
        }
        dst++;
    }
    entity_count = dst;
}

// --- PUBLIC API ---

EntityID entity_create(EntityType type)
{
    Entity *e = get_free_entity_slot();
    e->meta.id = next_entity_id++;
    e->meta.state = ES_INITILIZED;
    e->type = type;
    e->user_data = malloc(type.user_data_size);
    memset(e->user_data, 0, type.user_data_size);

    if (type.methods.init)
    {
        type.methods.init(e);
    }

    e->meta.state = ES_ACTIVE;
    entity_map_insert(e->meta.id, entity_count - 1);

    return e->meta.id;
}

bool entity_queue_free(EntityID id)
{
    if (id == ENTITY_INVALID_ID)
    {
        return false;
    }

    ssize_t map_idx = entity_map_find(id);
    if (map_idx < 0)
    {
        return false;
    }

    size_t entity_idx = entity_map[map_idx].index;
    if (entity_idx >= entity_count)
    {
        return false;
    }

    Entity *e = &entities[entity_idx];
    if (e->meta.state != ES_ACTIVE)
    {
        return false;
    }

    e->meta.state = ES_QUEUED_FREE;
    return true;
}

// --- PLUGIN LIFECYCLE ---

int init(CoreContext *ctx)
{
    entity_capacity = INITIAL_ENTITY_CAPACITY;
    entity_count = 0;
    entities = malloc(sizeof(Entity) * entity_capacity);

    entity_map_capacity = INITIAL_ENTITY_MAP_CAPACITY;
    entity_map_count = 0;
    entity_map = malloc(sizeof(EntityMapEntry) * entity_map_capacity);
    entity_map_clear(entity_map, entity_map_capacity);

    CC_BIND(ctx, CC_ENTITY_CREATE, entity_create, sizeof(entity_create), false);
    CC_BIND(ctx, CC_ENTITY_QUEUE_FREE, entity_queue_free, sizeof(entity_queue_free), false);
    return 0;
}

int update(CoreContext *ctx)
{
    (void)ctx;
    for (size_t i = 0; i < entity_count; ++i)
    {
        Entity *e = &entities[i];

        if (e->meta.state == ES_ACTIVE)
        {
            if (e->type.methods.update)
            {
                EntityState next_state = e->type.methods.update(e);
                e->meta.state = next_state; // <- normal update
            }
        }

        if (e->meta.state >= ES_CUSTOM)
        {
            uint64_t index = (uint64_t)(e->meta.state - ES_CUSTOM);

            if (index < e->type.methods.extra.len)
            {
                entity_method_fn_t extra_fn = e->type.methods.extra.funcs[index];
                if (extra_fn)
                {
                    EntityState next_state = extra_fn(e);
                    e->meta.state = next_state; // <- capture and assign new state from extra
                }
            }
            else
            {
                ctx->log(LL_ERROR, "Entity (%llu) [type:%s] has invalid custom state index %llu!", e->meta.id, e->type.name, index);
            }
        }
    }
    flush_destroyed_entities();
    return 0;
}

int shutdown(CoreContext *ctx)
{
    (void)ctx;

    for (size_t i = 0; i < entity_count; ++i)
    {
        Entity *e = &entities[i];
        if (e->meta.state != ES_FREED)
        {
            if (e->type.methods.shutdown)
            {
                e->type.methods.shutdown(e);
            }
            free(e->user_data);
            e->meta.state = ES_FREED;
        }
    }
    free(entities);
    free(entity_map);
    entities = NULL;
    entity_map = NULL;
    entity_capacity = 0;
    entity_count = 0;
    entity_map_capacity = 0;
    entity_map_count = 0;

    return 0;
}

// --- PLUGIN API ---

static const char *deps[] = {NULL};
static const char *optional[] = {NULL};

static PluginMetadata meta = {
    .name = "Entities",
    .required_deps = deps,
    .optional_deps = optional};

PluginAPI Load()
{
    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta};
}
