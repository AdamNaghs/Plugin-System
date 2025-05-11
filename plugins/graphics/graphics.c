#include "graphics.h"
#include "../signals/signals.h"

void renderable_draw(const Renderable* r)
{
    if (r->type == RENDERABLE_2D) {
        if (r->r2d.texture) {
            DrawTextureEx(*(r->r2d.texture), r->r2d.position, r->r2d.rotation, r->r2d.scale, r->r2d.tint);
        }
    }
    else if (r->type == RENDERABLE_3D) {
        if (r->r3d.model) {
            DrawModelEx(*(r->r3d.model), r->r3d.position, r->r3d.rotation, 1.0f, r->r3d.scale, r->r3d.tint);
        }
    }
}

static signal_emit_fn_t signal_emit_fn;

int init(CoreContext *ctx)
{
    (void)ctx;
    signal_emit_fn = CC_GET(ctx,CC_SIGNAL_EMIT);
    InitWindow(800, 600, "Test");
    return 0;
}

int update(CoreContext *ctx)
{
    if (WindowShouldClose() || IsKeyPressed(KEY_SPACE))
    {
        int *ptr = CC_GET(ctx, "CORE_SHOULD_RUN");
        *ptr = 0;
    }

    BeginDrawing();
    ClearBackground(WHITE);
    (*signal_emit_fn)(ctx,"graphics::draw_signal",NULL,NULL);
    EndDrawing();
    return 0;
}

int shutdown(CoreContext *ctx)
{
    (void)ctx;
    CloseWindow();
    return 0;
}

static const char* deps[] = { "Signals",NULL };
static const char* optional[] = { NULL };
static PluginMetadata meta = {"Graphics", deps, optional};

PluginAPI Load()
{
    return (PluginAPI){init, update, shutdown, &meta};
}