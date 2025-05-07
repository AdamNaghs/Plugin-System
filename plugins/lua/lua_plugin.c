#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../../include/plugin.h"
#include "../../include/dt.h"

static lua_State *L = NULL;

static int lua_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    CoreContext *ctx = lua_touserdata(L, lua_upvalueindex(1));
    ctx->log(LL_INFO, "[lua] %s", msg);
    return 0;
}


static int update_ref = LUA_NOREF;

int init(CoreContext *ctx)
{
    L = luaL_newstate();
    CC_BIND(ctx,"lua::state",L,sizeof(L),false);
    luaL_openlibs(L);

    // Example: bind log to Lua
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_log, 1);
    lua_setglobal(L, "log");

    // Optional: load an initial script
    if (luaL_dofile(L, "scripts/init.lua") != LUA_OK)
    {
        ctx->log(LL_ERROR, "Lua error: %s", lua_tostring(L, -1));
        return 1;
    }

    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1))
    {
        update_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        lua_pop(L, 1); // not a function, discard
    }

    return 0;
}

void run_lua_func(CoreContext *ctx, char *function)
{
    lua_getglobal(L, function);
    if (lua_isfunction(L, -1))
    {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK)
        {
            ctx->log(LL_ERROR, "[lua::%s] %s", function, lua_tostring(L, -1));
            lua_pop(L, 1); // pop error
            return;
        }
    }
    else
    {
        lua_pop(L, 1); // not a function
    }
}

int shutdown(CoreContext *ctx)
{
    run_lua_func(ctx, "shutdown");
    if (update_ref != LUA_NOREF)
    {
        luaL_unref(L, LUA_REGISTRYINDEX, update_ref);
    }
    lua_close(L);
    return 0;
}

int update(CoreContext *ctx)
{
    if (update_ref != LUA_NOREF)
    {
        lua_rawgeti(L, LUA_REGISTRYINDEX, update_ref);
        lua_pushnumber(L, ctx->delta_time);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK)
        {
            ctx->log(LL_ERROR, "[lua::update] %s", lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }

    return 0;
}

PluginAPI Load()
{
    static const char *deps[] = {NULL};
    static const char *optional[] = {NULL};
    static PluginMetadata meta = {"Lua", deps, optional};

    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta};
}
