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
    ES_CUSTOM
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
        entity_method_fn_t* funcs; /* should be static */
    } extra;
} EntityMethods;

typedef struct EntityType
{
    const char* name;
    EntityMethods methods;
    uint64_t user_data_size;
} EntityType;

#define ENTITY_TYPE(type_name, init_fn, update_fn, shutdown_fn, ...) \
    static entity_method_fn_t type_name##_extra_methods[] = { __VA_ARGS__ }; \
    static EntityType type_name = { \
        .name = #type_name, \
        .methods = { \
            .init = init_fn, \
            .update = update_fn, \
            .shutdown = shutdown_fn, \
            .extra = { \
                .len = sizeof(type_name##_extra_methods)/sizeof(entity_method_fn_t), \
                .funcs = type_name##_extra_methods \
            } \
        }, \
        .user_data_size = sizeof(struct type_name##_UserData) \
    }


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