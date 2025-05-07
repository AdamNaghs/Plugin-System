#include "../include/plugin.h"
#include "raylib.h"
#include <stdio.h>

int init(CoreContext *ctx)
{
    (void)ctx;
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
    DrawText("Hello World!", 400, 300, 20, BLACK);
    DrawFPS(0, 0);
    EndDrawing();
    return 0;
}

int shutdown(CoreContext *ctx)
{
    (void)ctx;
    CloseWindow();
    return 0;
}

static const char* deps[] = { NULL };
static const char* optional[] = { NULL };
static PluginMetadata meta = {"Graphics", deps, optional};

PluginAPI Load()
{
    return (PluginAPI){init, update, shutdown, &meta};
}