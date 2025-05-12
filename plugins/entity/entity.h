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

/* Order matters for extra functions being used with custom states */
#define ENTITY_TYPE(entity_var_name, struct_type_name, init_fn, update_fn, shutdown_fn, ...) \
    static entity_method_fn_t entity_var_name##_extra_methods[] = { __VA_ARGS__ }; \
    static EntityType entity_var_name = { \
        .name = #entity_var_name, \
        .methods = { \
            .init = init_fn, \
            .update = update_fn, \
            .shutdown = shutdown_fn, \
            .extra = { \
                .len = sizeof(entity_var_name##_extra_methods)/sizeof(entity_method_fn_t), \
                .funcs = entity_var_name##_extra_methods \
            } \
        }, \
        .user_data_size = sizeof(struct struct_type_name) \
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