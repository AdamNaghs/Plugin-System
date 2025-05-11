#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "../../include/plugin.h"
#include "../../include/dt.h"
#include "../signals/signals.h"

static lua_State *L = NULL;
static CoreContext *lua_ctx = NULL;

static void (*signal_emit_fn)(CoreContext*, const char*, void*, void*) = NULL;
static void (*signal_connect_fn)(const char*, SignalCallback, void*) = NULL;

static int update_ref = LUA_NOREF;

// ====================================================
// Core exposed functions
// ====================================================

/**
 * @brief Log a message from Lua.
 *
 * Lua Usage:
 *     core.log(message)
 */
static int lua_log(lua_State *L)
{
    const char *msg = luaL_checkstring(L, 1);
    CoreContext *ctx = lua_touserdata(L, lua_upvalueindex(1));
    ctx->log(LL_INFO, "[lua] %s", msg);
    return 0;
}

/**
 * @brief Emit a signal from Lua.
 *
 * Lua Usage:
 *     core.signal_emit(name,sender,args)
 * 
 * 
 */
static int lua_signal_emit(lua_State* L)
{
    const char* signal_name = luaL_checkstring(L, 1);

    void* sender = NULL;
    void* args = NULL;

    if (!signal_emit_fn || !lua_ctx)
        return 0;

    if (!lua_isnoneornil(L, 2))
        sender = lua_topointer(L, 2); // cast any Lua object to a unique pointer (safe identity)

    if (!lua_isnoneornil(L, 3))
        args = lua_topointer(L, 3);

    signal_emit_fn(lua_ctx, signal_name, sender, args);

    return 0;
}


/**
 * @brief Structure to hold Lua callback references for signals.
 */
typedef struct LuaSignalConnection
{
    lua_State* L;
    int ref;
} LuaSignalConnection;

/**
 * @brief C dispatcher to call Lua function when signal is triggered.
 */
static void lua_signal_dispatcher(CoreContext* ctx, void* sender, void* args, void* user_data)
{
    (void)ctx;
    (void)sender;
    (void)args;
    LuaSignalConnection* conn = (LuaSignalConnection*)user_data;
    lua_State* L = conn->L;

    lua_rawgeti(L, LUA_REGISTRYINDEX, conn->ref);

    lua_pushnil(L); // sender placeholder
    lua_pushnil(L); // args placeholder

    if (lua_pcall(L, 2, 0, 0) != LUA_OK)
    {
        printf("[lua_signal_dispatcher] Lua error: %s\n", lua_tostring(L, -1));
        lua_pop(L, 1); // pop error
    }
}

/**
 * @brief Connect a Lua function to a signal.
 *
 * Lua Usage:
 *     core.signal_connect(name, function(sender, args))
 */
static int lua_signal_connect(lua_State* L)
{
    const char* signal_name = luaL_checkstring(L, 1);
    luaL_checktype(L, 2, LUA_TFUNCTION);

    if (!signal_connect_fn) return 0;

    LuaSignalConnection* conn = malloc(sizeof(LuaSignalConnection));
    conn->L = L;
    lua_pushvalue(L, 2); // copy function
    conn->ref = luaL_ref(L, LUA_REGISTRYINDEX);

    signal_connect_fn(signal_name, lua_signal_dispatcher, conn);

    return 0;
}

static int lua_memory_get(lua_State* L)
{
    const char* key = luaL_checkstring(L, 1);

    if (!lua_ctx)
        return 0;

    void* ptr = CC_GET(lua_ctx, key);
    if (!ptr)
    {
        lua_pushnil(L);
        return 1;
    }

    int value = *(int*)ptr;
    lua_pushinteger(L, value);
    return 1;
}


// ====================================================
// Internal utilities
// ====================================================

/**
 * @brief Run a named Lua function (e.g., "init", "shutdown").
 */
void run_lua_func(CoreContext *ctx, const char *function)
{
    lua_getglobal(L, function);
    if (lua_isfunction(L, -1))
    {
        if (lua_pcall(L, 0, 0, 0) != LUA_OK)
        {
            ctx->log(LL_ERROR, "[lua::%s] %s", function, lua_tostring(L, -1));
            lua_pop(L, 1);
        }
    }
    else
    {
        lua_pop(L, 1); // not a function
    }
}

// ====================================================
// Plugin API functions
// ====================================================

/**
 * @brief Initialize the Lua plugin.
 */
int init(CoreContext *ctx)
{
    L = luaL_newstate();
    CC_BIND(ctx, "lua::state", L, sizeof(L), false);
    luaL_openlibs(L);

    lua_ctx = ctx;
    signal_emit_fn = CC_GET(ctx, "signal::emit");
    signal_connect_fn = CC_GET(ctx, "signal::connect");

    // Create 'core' table
    lua_newtable(L);
    lua_setglobal(L, "core");

    // Push core table
    lua_getglobal(L, "core");

    // Bind core.log
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_log, 1);
    lua_setfield(L, -2, "log");

    // Bind core.signal_emit
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_signal_emit, 1);
    lua_setfield(L, -2, "signal_emit");

    // Bind core.signal_connect
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_signal_connect, 1);
    lua_setfield(L, -2, "signal_connect");

    // Bind core.get
    lua_pushlightuserdata(L, ctx);
    lua_pushcclosure(L, lua_memory_get, 1);
    lua_setfield(L, -2, "get");

    // Pop core table
    lua_pop(L, 1);

    // Load scripts/init.lua
    if (luaL_dofile(L, "scripts/init.lua") != LUA_OK)
    {
        ctx->log(LL_ERROR, "Lua error: %s", lua_tostring(L, -1));
        lua_pop(L, 1); // remove error
        return 1;
    }

    // Capture optional update()
    lua_getglobal(L, "update");
    if (lua_isfunction(L, -1))
    {
        update_ref = luaL_ref(L, LUA_REGISTRYINDEX);
    }
    else
    {
        lua_pop(L, 1);
    }

    run_lua_func(ctx, "init");

    return 0;
}

/**
 * @brief Update the Lua plugin each frame.
 */
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

/**
 * @brief Shutdown the Lua plugin.
 */
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

/**
 * @brief Plugin entry point.
 */
PluginAPI Load()
{
    static const char *deps[] = { "Signals", NULL };
    static const char *optional[] = { NULL };
    static PluginMetadata meta = { "Lua", deps, optional };

    return (PluginAPI){
        .init = init,
        .update = update,
        .shutdown = shutdown,
        .meta = &meta
    };
}
