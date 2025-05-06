#include "../include/core.h"
#include <stdio.h>


void core_init(Core* core, int version, char* plugin_folder)
{
    core_context_new(&core->context,version);
    plugin_manager_new(&core->manager,plugin_folder);
    int *should_run = CC_ALLOC(&core->context,"CORE_SHOULD_RUN",sizeof(int));
    *should_run = 1;
    plugin_manager_init(&core->manager,&core->context);
    
}


void core_run(Core* core)
{
    int *should_run = CC_GET(&core->context,"CORE_SHOULD_RUN");
    while (should_run && *should_run) {
        core_context_update(&core->context);
        plugin_manager_update(&core->manager,&core->context);
    }
}


void core_shutdown(Core* core)
{
    plugin_manager_shutdown(&core->manager,&core->context);
    plugin_manager_free(&core->manager);
    core_context_free(&core->context);
}