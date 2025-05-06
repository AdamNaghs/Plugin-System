#include "../include/plugin.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAX_DEPENDENCIES 64
#define MAX_PLUGIN_COUNT 64

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
            //plugin->name[strlen(plugin->name)-3] = 0; /* remove ".so" from name */
            printf("Found Plugin: %s\n", plugin->name);
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

typedef struct Node {
    const char* name;
    Plugin* plugin;
    const char** depends_on;
    int visited;
    int visiting;
} Node;

static Node* find_node(Node* nodes, int count, const char* name) {
    for (int i = 0; i < count; i++) {
        if (strcmp(nodes[i].name, name) == 0)
            return &nodes[i];
    }
    return NULL;
}

static int dfs(Node* node, Node** sorted, int* index, Node* nodes, int total) {
    if (node->visiting) {
        printf("Cyclic dependency detected at %s\n", node->name);
        return 0;
    }
    if (node->visited) return 1;

    node->visiting = 1;

    for (const char** dep = node->depends_on; dep && *dep; dep++) {
        Node* dep_node = find_node(nodes, total, *dep);
        if (!dep_node) {
            printf("Missing dependency: %s for plugin %s\n", *dep, node->name);
            return 0;
        }
        if (!dfs(dep_node, sorted, index, nodes, total)) return 0;
    }

    node->visited = 1;
    node->visiting = 0;
    sorted[(*index)--] = node;

    return 1;
}

int sort_plugins_by_dependency(PluginManager* pm, Plugin** sorted_out) {
    Node nodes[MAX_PLUGIN_COUNT] = {0};
    Node* sorted[MAX_PLUGIN_COUNT];
    int total = (int)pm->plugins.len;
    int index = total - 1;

    // Create nodes
    for (int i = 0; i < total; i++) {
        Plugin* p = &pm->plugins.list[i];
        nodes[i].name = p->api->meta->name;
        nodes[i].depends_on = p->api->meta->depends_on;
        nodes[i].plugin = p;
    }

    // Sort
    for (int i = 0; i < total; i++) {
        if (!nodes[i].visited) {
            if (!dfs(&nodes[i], sorted, &index, nodes, total)) {
                return 0; // Failed
            }
        }
    }

    // Copy sorted plugins
    for (int i = 0; i < total; i++) {
        sorted_out[i] = sorted[i]->plugin;
    }

    return 1; // Success
}

void plugin_manager_init(PluginManager* pm, CoreContext* ctx)
{
    Plugin* sorted_plugins[MAX_PLUGIN_COUNT];
    if (!sort_plugins_by_dependency(pm, sorted_plugins)) {
        fprintf(stderr, "Plugin dependency sorting failed\n");
        exit(1);
    }
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        printf("Loading Plugin: %s\n", pm->plugins.list[i].api->meta->name);
        if (sorted_plugins[i]->api->init)
        sorted_plugins[i]->api->init(ctx);
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
void plugin_manager_shutdown(PluginManager* pm, CoreContext* ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->shutdown)
            pm->plugins.list[i].api->shutdown(ctx);
    }
}