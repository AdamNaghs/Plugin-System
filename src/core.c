#include "../include/core.h"

void core_init(Core* core, int version, char* plugin_folder)
{
    core_context_new(&core->context,version);
    PluginManager_New(&core->manager,plugin_folder);
    core->should_run = 1;
    core->context.memory.set(core->context.memory.map,"CORE_SHOULD_RUN",&core->should_run,sizeof(core->should_run));
    PluginManager_Init(&core->manager,&core->context);
}

void core_run(Core* core)
{
    while (core->should_run) {
        core_context_update(&core->context);
        PluginManager_Update(&core->manager,&core->context);
        PluginManager_Physics(&core->manager,&core->context);
        PluginManager_Draw(&core->manager,&core->context);
    }
}


void core_shutdown(Core* core)
{
    PluginManager_Shutdown(&core->manager,&core->context);
    core_context_free(&core->context);
    PluginManager_Free(&core->manager);
}