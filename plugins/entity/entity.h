#ifndef _ENTITY_H
#define _ENTITY_H

#include <stdbool.h>
#include <stdint.h>

#define CC_ENTITY_CREATE "entity::create"

#define CC_ENTITY_QUEUE_FREE "entity::queue_free"

#define ENTITY_INVALID_ID 0

typedef uint64_t EntityID;

typedef enum EntityState
{
    ES_UNINITILIZED,
    ES_INITILIZED,
    ES_ACTIVE,
    ES_QUEUED_FREE,
    ES_FREED,
} EntityState;

typedef struct EntityMetadata
{
    EntityID id;
    EntityState state;
} EntityMetadata;

typedef struct Entity Entity;

typedef EntityState (*entity_method_fn_t)(Entity* self);

typedef struct EntityMethods
{
    entity_method_fn_t init;
    entity_method_fn_t update;
    entity_method_fn_t shutdown;
    struct {
        uint64_t len;
        entity_method_fn_t* funcs;
    } extra;
} EntityMethods;

typedef struct EntityType
{
    const char* name;
    EntityMethods methods;
    uint64_t user_data_size;
} EntityType;

struct Entity
{
    EntityMetadata meta;
    EntityType type;
    void* user_data;
};

EntityID entity_create(EntityType type);

typedef EntityID (*entity_create_fn_t)(EntityType type);

bool entity_queue_free(EntityID id);

typedef bool (*entity_queue_free_fn_t)(EntityID id);

#endif /* _ENTITY_H */