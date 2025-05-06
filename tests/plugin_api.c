#include "../include/plugin.h"
#include "raylib.h"
#include <stdio.h>

int init(CoreContext* ctx)
{
    InitWindow(800,600,"Test");
    return 0;
}

int update(CoreContext* ctx)
{
    if (WindowShouldClose() || IsKeyPressed(KEY_SPACE))
    {
        int *ptr = CTX_GET(ctx,"CORE_SHOULD_RUN");
        *ptr = 0;
    }
    return 0;
}

int draw(CoreContext* ctx)
{
    BeginDrawing();
    ClearBackground(WHITE);
    DrawText("Hello World!",400,300,20,BLACK);
    DrawFPS(0,0);
    EndDrawing();
    return 0;
}

int shutdown(CoreContext* ctx)
{
    CloseWindow();
    return 0;
}

PluginAPI Load()
{
    return (PluginAPI){init,update,draw,shutdown};
}