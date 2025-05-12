#include "../../include/plugin_api.h"
#include "../graphics/graphics.h" // For CC_GRAPHICS_DRAW_SIGNAL
#include "../entity/entity.h"     // For entity_create, EntityType, etc.
#include "../signals/signals.h"   // For signal_connect

#include <stdlib.h>

// Plugin Dependancies
static entity_create_fn_t entity_create_fn;
static entity_queue_free_fn_t entity_queue_free_fn;
static signal_connect_fn_t signal_connect_fn;

typedef struct
{
    Vector2 position;
    Texture2D *texture;
} PlayerData;

static EntityType PlayerEntityType;

static EntityID player_id = ENTITY_INVALID_ID;

// -- Player Behavior --

static void player_draw(CoreContext *ctx, void *sender, void *args, void *user_data)
{
    (void)ctx;
    (void)sender;
    (void)args;
    Entity *self = (Entity *)user_data;
    PlayerData *data = (PlayerData *)self->user_data;

    if (data->texture)
    {
        DrawTextureEx(*data->texture, data->position, 0.0f, 1.0f, WHITE);
    }
    else
    {
        DrawCircleV(data->position, 20, RED); // Placeholder if no texture
    }
}

static EntityState player_init(Entity *self)
{
    PlayerData *data = (PlayerData *)self->user_data;
    data->position = (Vector2){100, 100};
    data->texture = NULL; // safe default

    // Connect to draw signal here
    (*signal_connect_fn)(CC_GRAPHICS_DRAW_SIGNAL, player_draw, self);

    return ES_INITILIZED;
}

static EntityState player_update(Entity *self)
{
    PlayerData *data = (PlayerData *)self->user_data;
    if (IsKeyDown(KEY_RIGHT))
        data->position.x += 100 * GetFrameTime();
    if (IsKeyDown(KEY_LEFT))
        data->position.x -= 100 * GetFrameTime();
    if (IsKeyDown(KEY_DOWN))
        data->position.y += 100 * GetFrameTime();
    if (IsKeyDown(KEY_UP))
        data->position.y -= 100 * GetFrameTime();
    return ES_ACTIVE;
}

// -- Plugin Lifecycle --

int init(CoreContext *ctx)
{
    entity_create_fn = CC_GET(ctx,CC_ENTITY_CREATE);
    entity_queue_free_fn = CC_GET(ctx,CC_ENTITY_QUEUE_FREE);
    signal_connect_fn = CC_GET(ctx, CC_SIGNAL_CONNECT);

    PlayerEntityType = (EntityType){
        .name = "Player",
        .methods = {
            .init = player_init,
            .update = player_update,
            .shutdown = NULL,
            .extra = {0, NULL}},
        .user_data_size = sizeof(PlayerData)};

    // Create player entity
    player_id = (*entity_create_fn)(PlayerEntityType);

    ctx->log(LL_INFO,"[Game] Initialized player entity (ID: %llu)\n", player_id);

    return 0;
}

int update(CoreContext *ctx)
{
    (void)ctx;
    // Game logic could go here (spawning enemies, checking win/lose)
    return 0;
}

int shutdown(CoreContext *ctx)
{
    (void)ctx;
    // (Optional) queue free the player
    if (player_id != ENTITY_INVALID_ID)
    {
        (*entity_queue_free_fn)(player_id);
    }
    return 0;
}

// -- Metadata --

static const char *deps[] = {"Graphics", "Signals", "Entities", NULL};
static const char *optional[] = {NULL};

static PluginMetadata meta = {
    .name = "Game",
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
