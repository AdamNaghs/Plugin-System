#include "../include/plugin.h"
#include "raylib.h"
#include <stdio.h>

int init(CoreContext* ctx)
{
    printf("Window was inited\n");
    InitWindow(800,600,"Test");
    return 0;
}

int update(CoreContext* ctx)
{
    if (WindowShouldClose())
    {
        int value = 0;
        MEM_CTX_SET_VALUE(ctx->memory.map,"CORE_SHOULD_RUN",value);
    }
    return 0;
}

int draw(CoreContext* ctx)
{
    BeginDrawing();
    ClearBackground(WHITE);
    DrawText("Hello World!",400,300,20,BLACK);
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
    return (PluginAPI){init,update,NULL,draw,shutdown};
}