#ifndef _PLUGIN_MANAGER_H
#define _PLUGIN_MANAGER_H

#include "plugin_api.h"

typedef struct Plugin
{
    PluginAPI* api;
    void* handle;
    char* name;
    int version;
} Plugin;

typedef struct PluginManager
{
    struct
    {
        Plugin* list;
        size_t len;
    } plugins;
    CoreContext* ctx;
} PluginManager;

void plugin_manager_new(PluginManager* pm, char* folder_path);
void plugin_manager_free(PluginManager* pm);

void plugin_manager_init(PluginManager* pm, CoreContext* ctx);
void plugin_manager_update(PluginManager* pm, CoreContext* ctx);
void plugin_manager_draw(PluginManager* pm, CoreContext* ctx);
void plugin_manager_shutdown(PluginManager* pm, CoreContext* ctx);



#endif /*_PLUGIN_MANAGER_H*/