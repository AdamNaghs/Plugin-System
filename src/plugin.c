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
#define DYNLIB_HANDLE void *
#define DYNLIB_OPEN(path) dlopen(path, RTLD_LAZY)
#define DYNLIB_SYM(handle, sym) dlsym(handle, sym)
#define DYNLIB_CLOSE(handle) dlclose(handle)
#endif

#define MAX_PLUGINS 64

void plugin_manager_new(PluginManager *pm, char *folder_path)
{
    pm->plugins.list = (Plugin *)calloc(MAX_PLUGINS, sizeof(Plugin));
    pm->plugins.len = 0;

    // Simplified file discovery for POSIX
#ifndef _WIN32
    DIR *dir = opendir(folder_path);
    if (!dir)
    {
        perror("opendir");
        exit(1);
        return;
    }

    struct dirent *entry;
    while ((entry = readdir(dir)))
    {
        if (strstr(entry->d_name, ".so"))
        {
            char fullpath[512];
            snprintf(fullpath, sizeof(fullpath), "%s/%s", folder_path, entry->d_name);

            DYNLIB_HANDLE handle = DYNLIB_OPEN(fullpath);
            if (!handle)
            {
                logger(LL_ERROR, "\t\tFailed to load plugin: %s", fullpath);
                continue;
            }

            PluginAPI (*load_func)() = (PluginAPI (*)())DYNLIB_SYM(handle, "Load");
            if (!load_func)
            {
                logger(LL_ERROR, "\t\tMissing Load() in plugin: %s", fullpath);
                DYNLIB_CLOSE(handle);
                continue;
            }

            Plugin *plugin = &pm->plugins.list[pm->plugins.len++];
            plugin->api = malloc(sizeof(PluginAPI));
            *plugin->api = load_func();
            plugin->handle = handle;
            plugin->name = strdup(entry->d_name);
            // plugin->name[strlen(plugin->name)-3] = 0; /* remove ".so" from name */
            logger(LL_INFO, "\t\tFound Plugin: %s", plugin->name);
        }
    }
    closedir(dir);
#else
// TODO: Use FindFirstFile / FindNextFile to scan DLLs on Windows
// Hardcode test plugin for now (or use globbing library)
#error "TODO: Implement plugin folder scanning on Windows using FindFirstFile"
    char *dll_path = "plugin.dll"; // Update this
    DYNLIB_HANDLE handle = DYNLIB_OPEN(dll_path);
    if (!handle)
        return;

    PluginAPI (*load_func)() = (PluginAPI (*)())DYNLIB_SYM(handle, "Load");
    if (!load_func)
    {
        DYNLIB_CLOSE(handle);
        return;
    }

    Plugin *plugin = &pm->plugins.list[pm->plugins.len++];
    plugin->api = malloc(sizeof(PluginAPI));
    *plugin->api = load_func();
    plugin->handle = handle;
    plugin->name = strdup("plugin.dll");
#endif
}

void plugin_manager_free(PluginManager *pm)
{
    for (size_t i = 0; i < pm->plugins.len; i++)
    {
        Plugin *plugin = &pm->plugins.list[i];
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

typedef struct Node
{
    const char *name;
    Plugin *plugin;
    const char **depends_on;    // required
    const char **optional_deps; // new
    int visited;
    int visiting;
} Node;

static Node *find_node(Node *nodes, int count, const char *name)
{
    for (int i = 0; i < count; i++)
    {
        if (strcmp(nodes[i].name, name) == 0)
            return &nodes[i];
    }
    return NULL;
}

static int dfs_with_optional(Node *node, Node **sorted, int *index, Node *nodes, int total)
{
    if (node->visiting)
    {
        logger(LL_ERROR, "\t\tCyclic dependency detected at plugin '%s'", node->name);
        return 0;
    }
    if (node->visited)
    {
        return 1;
    }

    node->visiting = 1;

    // First handle required dependencies
    for (const char **dep = node->depends_on; dep && *dep; dep++)
    {
        Node *dep_node = find_node(nodes, total, *dep);
        if (!dep_node)
        {
            logger(LL_ERROR, "\t\tMissing REQUIRED dependency '%s' for plugin '%s'", *dep, node->name);
            return 0;
        }
        if (!dfs_with_optional(dep_node, sorted, index, nodes, total))
        {
            return 0;
        }
    }

    // Then handle optional dependencies
    for (const char **opt = node->optional_deps; opt && *opt; opt++)
    {
        Node *opt_node = find_node(nodes, total, *opt);
        if (opt_node)
        {
            if (!dfs_with_optional(opt_node, sorted, index, nodes, total))
            {
                return 0;
            }
        }
        else
        {
            logger(LL_WARN, "\t\tOptional dependency '%s' missing for plugin '%s'", *opt, node->name);
        }
    }

    node->visited = 1;
    node->visiting = 0;
    sorted[(*index)++] = node;

    return 1;
}


int sort_plugins_by_dependency(PluginManager *pm, Plugin **sorted_out)
{
    Node nodes[MAX_PLUGIN_COUNT] = {0};
    Node *sorted[MAX_PLUGIN_COUNT];
    int total = (int)pm->plugins.len;
    int index = 0;

    // Create nodes
    for (int i = 0; i < total; i++)
    {
        Plugin *p = &pm->plugins.list[i];
        nodes[i].name = p->api->meta->name;
        nodes[i].depends_on = p->api->meta->required_deps;
        nodes[i].optional_deps = p->api->meta->optional_deps;
        nodes[i].plugin = p;
    }

    // Modified DFS to handle both required and optional dependencies
    for (int i = 0; i < total; i++)
    {
        if (!nodes[i].visited)
        {
            if (!dfs_with_optional(&nodes[i], sorted, &index, nodes, total))
            {
                return 0; // Failed
            }
        }
    }

    // Copy sorted plugins into output
    for (int i = 0; i < total; i++)
    {
        sorted_out[i] = sorted[i]->plugin;
    }

    return 1; // Success
}


void plugin_manager_hot_reload(PluginManager* pm, CoreContext* ctx)
{
    ctx->log(LL_WARN,"Hot reloading plugins...\n");

    // Shutdown and free existing plugins
    plugin_manager_shutdown(pm, ctx);
    plugin_manager_free(pm);

    // Recreate fresh empty plugin list
    plugin_manager_new(pm, "./build/plugins"); // or whatever your folder is

    // Init them again
    plugin_manager_init(pm, ctx);

    ctx->log(LL_INFO,"Hot reload complete.\n");
}


void plugin_manager_init(PluginManager *pm, CoreContext *ctx)
{
    Plugin *sorted_plugins[MAX_PLUGIN_COUNT];
    if (!sort_plugins_by_dependency(pm, sorted_plugins))
    {
        ctx->log(LL_ERROR, "\t\tPlugin dependency sorting failed");
        exit(1);
    }

    // Create a new plugin list in sorted order
    Plugin *new_list = malloc(sizeof(Plugin) * pm->plugins.len);
    size_t i;
    for (i = 0; i < pm->plugins.len; ++i)
    {
        new_list[i] = *sorted_plugins[i]; // copy struct contents
    }

    // Replace the old list
    free(pm->plugins.list);
    pm->plugins.list = new_list;

    for (i = 0; i < pm->plugins.len; i++) //  load most depended plugins first
    {
        ctx->log(LL_INFO, "\t\tLoading Plugin: %s", pm->plugins.list[i].api->meta->name);
        if (pm->plugins.list[i].api->init)
            pm->plugins.list[i].api->init(ctx);
    }
}

void plugin_manager_update(PluginManager *pm, CoreContext *ctx)
{
    size_t i;
    for (i = 0; i < pm->plugins.len; i++)
    {
        if (pm->plugins.list[i].api->update)
            pm->plugins.list[i].api->update(ctx);
    }
}
void plugin_manager_shutdown(PluginManager *pm, CoreContext *ctx)
{
    ctx->log(LL_INFO, "\t\tPlugin Shutdown:");
    size_t i;
    for (i = pm->plugins.len; i-- > 0;) //  unload most connected plugins first
    {
        if (pm->plugins.list[i].api->shutdown)
        {
            pm->plugins.list[i].api->shutdown(ctx);
            ctx->log(LL_INFO, "\t\t+\t%s \tsuccessfully shutdown.", pm->plugins.list[i].api->meta->name);
        }
        else
        {
            ctx->log(LL_INFO, "\t\t-\t%s \tmissing shutdown.", pm->plugins.list[i].api->meta->name);
        }
    }
    ctx->log(LL_INFO, "\t\t\tShutdown %zu plugins.", pm->plugins.len);
}