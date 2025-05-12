#include "graphics.h"
#include "../signals/signals.h"

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
    (*signal_emit_fn)(ctx,CC_GRAPHICS_DRAW_SIGNAL,NULL,NULL);
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