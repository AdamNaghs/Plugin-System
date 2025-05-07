#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../../include/plugin.h"
#include "../../include/dt.h"

static lua_State* L = NULL;

static int lua_log(lua_State* L) {
    const char* msg = luaL_checkstring(L, 1);
    CoreContext* ctx = lua_touserdata(L, lua_upvalueindex(1));
    ctx->log(LL_INFO, "[lua] %s", msg);
    return 0;
}

int init(CoreContext* ctx) {
    L = luaL_newstate();
    luaL_openlibs(L);

    // Example: bind log to Lua
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_log, 1);
    lua_setglobal(L, "log");

    // Optional: load an initial script
    if (luaL_dofile(L, "scripts/init.lua") != LUA_OK) {
        ctx->log(LL_ERROR, "Lua error: %s", lua_tostring(L, -1));
        return 1;
    }

    return 0;
}

void run_lua_func(CoreContext* ctx, char* function)
{
    lua_getglobal(L, function);
    if (lua_isfunction(L, -1)) {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK) {
            ctx->log(LL_ERROR, "[lua::%s] %s", function,lua_tostring(L, -1));
            lua_pop(L, 1); // pop error
            return;
        }
    } else {
        lua_pop(L, 1); // not a function
    }

}

void run_lua_func1f(CoreContext* ctx, const char* func, float arg) {
    lua_getglobal(L, func);
    if (lua_isfunction(L, -1)) {
        lua_pushnumber(L, arg);
        if (lua_pcall(L, 1, 0, 0) != LUA_OK) {
            ctx->log(LL_ERROR, "[lua::%s] %s", func, lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    } else {
        lua_pop(L, 1);
    }
}


int shutdown(CoreContext* ctx) {
    run_lua_func(ctx,"shutdown");
    lua_close(L);
    return 0;
}

int update(CoreContext* ctx) {
    run_lua_func1f(ctx, "update", get_dt());
    return 0;
}


PluginAPI Load() {
    static const char* deps[] = { NULL };
    static const char* optional[] = { NULL };
    static PluginMetadata meta = { "Lua", deps, optional };

    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta
    };
}
