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
#define DYNLIB_HANDLE void *
#define DYNLIB_OPEN(path) dlopen(path, RTLD_LAZY)
#define DYNLIB_SYM(handle, sym) dlsym(handle, sym)
#define DYNLIB_CLOSE(handle) dlclose(handle)
#endif

void plugin_manager_new(PluginManager *pm, char *folder_path)
{
    pm->plugins.list = NULL;
    pm->plugins.len = 0;
    size_t capacity = 0;

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

            // Grow plugin list dynamically
            if (pm->plugins.len >= capacity)
            {
                capacity = (capacity == 0) ? 8 : capacity * 2;
                Plugin *new_list = realloc(pm->plugins.list, sizeof(Plugin) * capacity);
                if (!new_list)
                {
                    logger(LL_ERROR, "Failed to realloc plugin list.");
                    exit(1);
                }
                pm->plugins.list = new_list;
            }

            Plugin *plugin = &pm->plugins.list[pm->plugins.len++];
            plugin->api = malloc(sizeof(PluginAPI));
            *plugin->api = load_func();
            plugin->handle = handle;
            plugin->name = strdup(entry->d_name);

            logger(LL_INFO, "\t\tFound Plugin: %s", plugin->name);
        }
    }
    closedir(dir);
#else
#error "TODO: Implement plugin folder scanning on Windows using FindFirstFile"
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
    int total = (int)pm->plugins.len;
    int index = 0;

    // Dynamically allocate nodes and sorted arrays
    Node *nodes = calloc(total, sizeof(Node));
    if (!nodes)
    {
        logger(LL_ERROR, "Failed to allocate memory for nodes.");
        return 0;
    }

    Node **sorted = calloc(total, sizeof(Node*));
    if (!sorted)
    {
        logger(LL_ERROR, "Failed to allocate memory for sorted nodes.");
        free(nodes);
        return 0;
    }

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
                free(nodes);
                free(sorted);
                return 0; // Failed
            }
        }
    }

    // Copy sorted plugins into output
    for (int i = 0; i < total; i++)
    {
        sorted_out[i] = sorted[i]->plugin;
    }

    free(nodes);
    free(sorted);

    return 1; // Success
}


void plugin_manager_hot_reload(PluginManager *pm, CoreContext *ctx)
{
    ctx->log(LL_WARN, "Hot reloading plugins...\n");

    // Shutdown and free existing plugins
    plugin_manager_shutdown(pm, ctx);
    plugin_manager_free(pm);

    // Recreate fresh empty plugin list
    plugin_manager_new(pm, "./build/plugins"); // or whatever your folder is

    // Init them again
    plugin_manager_init(pm, ctx);

    ctx->log(LL_INFO, "Hot reload complete.\n");
}

void plugin_manager_init(PluginManager *pm, CoreContext *ctx)
{
    size_t total = pm->plugins.len;

    // Dynamically allocate sorted_plugins
    Plugin **sorted_plugins = calloc(total, sizeof(Plugin *));
    if (!sorted_plugins)
    {
        ctx->log(LL_ERROR, "\t\tFailed to allocate memory for plugin sorting");
        exit(1);
    }

    if (!sort_plugins_by_dependency(pm, sorted_plugins))
    {
        ctx->log(LL_ERROR, "\t\tPlugin dependency sorting failed");
        free(sorted_plugins);
        exit(1);
    }

    // Create a new plugin list in sorted order
    Plugin *new_list = malloc(sizeof(Plugin) * total);
    if (!new_list)
    {
        ctx->log(LL_ERROR, "\t\tFailed to allocate memory for plugin list");
        free(sorted_plugins);
        exit(1);
    }

    for (size_t i = 0; i < total; ++i)
    {
        new_list[i] = *sorted_plugins[i]; // copy struct contents
    }

    free(sorted_plugins);

    // Replace the old list
    free(pm->plugins.list);
    pm->plugins.list = new_list;

    // Initialize plugins in sorted order
    for (size_t i = 0; i < pm->plugins.len; i++)
    {
        ctx->log(LL_INFO, "\t\tLoading Plugin: %s", pm->plugins.list[i].api->meta->name);
        if (pm->plugins.list[i].api->init)
        {
            pm->plugins.list[i].api->init(ctx);
        }
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