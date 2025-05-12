#include "../../include/plugin_api.h"
#include "../graphics/graphics.h"
#include "../entity/entity.h"
#include "../signals/signals.h"

// -- Plugin Dependencies --

static entity_create_fn_t entity_create_fn;
static entity_queue_free_fn_t entity_queue_free_fn;
static signal_connect_fn_t signal_connect_fn;
static logger_fn_t log;

static void load_plugins(CoreContext *ctx)
{
    entity_create_fn = CC_GET(ctx, CC_ENTITY_CREATE);
    entity_queue_free_fn = CC_GET(ctx, CC_ENTITY_QUEUE_FREE);
    signal_connect_fn = CC_GET(ctx, CC_SIGNAL_CONNECT);
    log = ctx->log;
}

// -- Player Custom States --

typedef enum PlayerState
{
    PLAYER_IDLE = ES_CUSTOM + 0,
    PLAYER_MOVING,
    PLAYER_DASHING,
    PLAYER_SHOOTING
} PlayerState;

// -- Player Data --

typedef struct PlayerEntity
{
    Vector2 position;
    Texture2D* texture;
} PlayerEntity;

// -- Forward Declarations --

static EntityState player_init(Entity* self);
static EntityState player_update(Entity* self);
static EntityState player_shutdown(Entity* self);
static void player_draw(CoreContext* ctx, void* sender, void* args, void* user_data);
static EntityState player_idle(Entity* self);
static EntityState player_move(Entity* self);
static EntityState player_dash(Entity* self);
static EntityState player_shoot(Entity* self);

// -- Player EntityType --

ENTITY_TYPE(PlayerEntityType, PlayerEntity, player_init, player_update, player_shutdown, player_idle, player_move, player_dash, player_shoot);

// -- Player State --

static EntityID player_id = ENTITY_INVALID_ID;

// -- Player Behavior Functions --

static void player_draw(CoreContext* ctx, void* sender, void* args, void* user_data)
{
    (void)ctx;
    (void)sender;
    (void)args;
    Entity* self = (Entity*)user_data;
    PlayerEntity* data = (PlayerEntity*)self->user_data;

    if (data->texture)
    {
        DrawTextureEx(*data->texture, data->position, 0.0f, 1.0f, WHITE);
    }
    else
    {
        DrawCircleV(data->position, 20, RED); // Placeholder
    }

    DrawFPS(0, 0);
}

static EntityState player_init(Entity* self)
{
    PlayerEntity* data = (PlayerEntity*)self->user_data;
    data->position = (Vector2){100, 100};
    data->texture = NULL;

    (*signal_connect_fn)(CC_GRAPHICS_DRAW_SIGNAL, player_draw, self);

    return ES_INITILIZED;
}

static EntityState player_shutdown(Entity* self)
{
    //log(LL_INFO, "Shutting down Player Entity.");
    PlayerEntity* data = (PlayerEntity*)self->user_data;
    (void)data; // nothing to free yet
    return ES_FREED;
}

static EntityState player_update(Entity* self)
{
    (void)self;
    //log(LL_INFO,"State=%d", self->meta.state);

    if (IsKeyPressed(KEY_SPACE))
    {
        return (EntityState)PLAYER_SHOOTING;
    }
    if (IsKeyDown(KEY_LEFT_SHIFT))
    {
        return (EntityState)PLAYER_DASHING;
    }
    if (IsKeyDown(KEY_RIGHT) || IsKeyDown(KEY_LEFT) || IsKeyDown(KEY_UP) || IsKeyDown(KEY_DOWN))
    {
        return (EntityState)PLAYER_MOVING;
    }
    return (EntityState)PLAYER_IDLE;
}

// -- Extra Methods for Player Custom States --

static EntityState player_idle(Entity* self)
{
    (void)self;
    return ES_ACTIVE;
}

static EntityState player_move(Entity* self)
{
    //log(LL_INFO,"State=MOVE");
    PlayerEntity* data = (PlayerEntity*)self->user_data;

    bool moving = false;

    if (IsKeyDown(KEY_RIGHT)) { data->position.x += 10 * GetFrameTime(); moving = true; }
    if (IsKeyDown(KEY_LEFT))  { data->position.x -= 10 * GetFrameTime(); moving = true; }
    if (IsKeyDown(KEY_DOWN))  { data->position.y += 10 * GetFrameTime(); moving = true; }
    if (IsKeyDown(KEY_UP))    { data->position.y -= 10 * GetFrameTime(); moving = true; }

    if (moving)
        return (EntityState)PLAYER_MOVING;
    else
        return (EntityState)PLAYER_IDLE;
}

static EntityState player_dash(Entity* self)
{
    //log(LL_INFO,"State=DASH");
    PlayerEntity* data = (PlayerEntity*)self->user_data;

    bool moving = false;

    if (IsKeyDown(KEY_RIGHT)) { data->position.x += 1000 * GetFrameTime(); moving = true; }
    if (IsKeyDown(KEY_LEFT))  { data->position.x -= 1000 * GetFrameTime(); moving = true; }
    if (IsKeyDown(KEY_DOWN))  { data->position.y += 1000 * GetFrameTime(); moving = true; }
    if (IsKeyDown(KEY_UP))    { data->position.y -= 1000 * GetFrameTime(); moving = true; }

    if (!IsKeyDown(KEY_LEFT_SHIFT))
    {
        if (moving)
            return (EntityState)PLAYER_MOVING;
        else
            return (EntityState)PLAYER_IDLE;
    }

    return (EntityState)PLAYER_DASHING;
}

static EntityState player_shoot(Entity* self)
{
    //log(LL_INFO,"State=SHOOT");
    PlayerEntity* data = (PlayerEntity*)self->user_data;
    (void)data; // shooting logic would go here eventually

    // After shooting, immediately go back to idle
    return (EntityState)PLAYER_IDLE;
}

// -- Plugin Lifecycle --

int init(CoreContext* ctx)
{
    load_plugins(ctx);
    player_id = (*entity_create_fn)(PlayerEntityType);
    return 0;
}

int update(CoreContext* ctx)
{
    (void)ctx;
    return 0;
}

int shutdown(CoreContext* ctx)
{
    (void)ctx;
    if (player_id != ENTITY_INVALID_ID)
    {
        (*entity_queue_free_fn)(player_id);
    }
    return 0;
}

// -- Metadata --

static const char* deps[] = {"Graphics", "Signals", "Entities", NULL};
static const char* optional[] = {NULL};

static PluginMetadata meta = {
    .name = "Game",
    .required_deps = deps,
    .optional_deps = optional
};

PluginAPI Load()
{
    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta
    };
}
