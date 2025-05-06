#include "../include/plugin.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#ifdef _WIN32
    #include <windows.h>
    #define DYNLIB_HANDLE HMODULE
    #define DYNLIB_OPEN(path) LoadLibraryA(path)
    #define DYNLIB_SYM(handle, sym) GetProcAddress(handle, sym)
    #define DYNLIB_CLOSE(handle) FreeLibrary(handle)
#else
    #include <dlfcn.h>
    #include <dirent.h>
    #define DYNLIB_HANDLE void*
    #define DYNLIB_OPEN(path) dlopen(path, RTLD_LAZY)
    #define DYNLIB_SYM(handle, sym) dlsym(handle, sym)
    #define DYNLIB_CLOSE(handle) dlclose(handle)
#endif

#define MAX_PLUGINS 64

void plugin_manager_new(PluginManager* pm, char* folder_path)
{
    pm->plugins.list = (Plugin*)calloc(MAX_PLUGINS, sizeof(Plugin));
    pm->plugins.len = 0;

    // Simplified file discovery for POSIX
#ifndef _WIN32
    DIR* dir = opendir(folder_path);
    if (!dir) {
        perror("opendir");
        exit(1);
        return;
    }

    struct dirent* entry;
    while ((entry = readdir(dir))) {
        printf("Scanning: %s\n", entry->d_name);
        if (strstr(entry->d_name, ".so")) {
            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", folder_path, entry->d_name);

            DYNLIB_HANDLE handle = DYNLIB_OPEN(fullpath);
            if (!handle) {
                fprintf(stderr, "Failed to load plugin: %s\n", fullpath);
                continue;
            }

            PluginAPI (*load_func)() = (PluginAPI (*)())DYNLIB_SYM(handle, "Load");
            if (!load_func) {
                fprintf(stderr, "Missing Load() in plugin: %s\n", fullpath);
                DYNLIB_CLOSE(handle);
                continue;
            }

            Plugin* plugin = &pm->plugins.list[pm->plugins.len++];
            plugin->api = malloc(sizeof(PluginAPI));
            *plugin->api = load_func();
            plugin->handle = handle;
            plugin->name = strdup(entry->d_name);
        }
    }
    closedir(dir);
#else
    // TODO: Use FindFirstFile / FindNextFile to scan DLLs on Windows
    // Hardcode test plugin for now (or use globbing library)
    #error "TODO: Implement plugin folder scanning on Windows using FindFirstFile"
    char* dll_path = "plugin.dll";  // Update this
    DYNLIB_HANDLE handle = DYNLIB_OPEN(dll_path);
    if (!handle) return;

    PluginAPI (*load_func)() = (PluginAPI (*)())DYNLIB_SYM(handle, "Load");
    if (!load_func) {
        DYNLIB_CLOSE(handle);
        return;
    }

    Plugin* plugin = &pm->plugins.list[pm->plugins.len++];
    plugin->api = malloc(sizeof(PluginAPI));
    *plugin->api = load_func();
    plugin->handle = handle;
    plugin->name = strdup("plugin.dll");
#endif
}

void plugin_manager_free(PluginManager* pm)
{
    for (size_t i = 0; i < pm->plugins.len; i++) {
        Plugin* plugin = &pm->plugins.list[i];
        if (plugin->handle)
            DYNLIB_CLOSE(plugin->handle);
        if (plugin->name)
            free(plugin->name);
        if (plugin->api)
            free(plugin->api);
    }
    free(pm->plugins.list);
    pm->plugins.list = NULL;
    pm->plugins.len = 0;
}


void plugin_manager_init(PluginManager* pm, CoreContext* ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->init)
            pm->plugins.list[i].api->init(ctx);
    }
}
void plugin_manager_update(PluginManager* pm, CoreContext* ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->update)
            pm->plugins.list[i].api->update(ctx);
    }
}
void plugin_manager_physics(PluginManager* pm, CoreContext* ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->physics)
            pm->plugins.list[i].api->physics(ctx);
    }
}
void plugin_manager_draw(PluginManager* pm, CoreContext* ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->draw)
            pm->plugins.list[i].api->draw(ctx);
    }
}
void plugin_manager_shutdown(PluginManager* pm, CoreContext* ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->shutdown)
            pm->plugins.list[i].api->shutdown(ctx);
    }
}