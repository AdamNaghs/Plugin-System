#include "../include/core.h"
#include <stdio.h>


void core_init(Core* core, int version, char* plugin_folder)
{
    logger_init();
    logger(LL_INFO,"Initializing Core...");
    logger(LL_INFO,"\tCreating Core Context");
    core_context_new(&core->context,version);
    logger(LL_INFO,"\tCreating Plugin Manager");
    plugin_manager_new(&core->manager,plugin_folder);
    /* Init core variables */
    int *should_run = CC_ALLOC(&core->context,"CORE_SHOULD_RUN",sizeof(int));
    int *should_hot_reload = CC_ALLOC(&core->context,"CORE_SHOULD_HOT_RELOAD",sizeof(int));
    *should_run = 1;
    *should_hot_reload = 0;
    
    logger(LL_INFO,"\tInitializing Plugins");
    plugin_manager_init(&core->manager,&core->context);
    logger(LL_INFO, "Core Initilized.");
    
}


void core_run(Core* core)
{
    int *should_run = CC_GET(&core->context,"CORE_SHOULD_RUN");
    int *should_hot_reload = CC_GET(&core->context,"COER_SHOULD_HOT_RELOAD");
    while (should_run && *should_run) {
        core_context_update(&core->context);
        plugin_manager_update(&core->manager,&core->context);
        if (should_hot_reload && *should_hot_reload)
        {
            plugin_manager_hot_reload(&core->manager,&core->context);
            *should_hot_reload = 0;
        }
    }
    logger(LL_INFO,"Core Loop Concluded.");
}


void core_shutdown(Core* core)
{
    logger(LL_INFO,"Shutting Down Core...");
    logger(LL_INFO,"\tShutingdown Plugins");
    plugin_manager_shutdown(&core->manager,&core->context);
    logger(LL_INFO,"\tFreeing Plugin Manager");
    plugin_manager_free(&core->manager);
    logger(LL_INFO,"\tFreeing Core Context");
    core_context_free(&core->context);
    logger(LL_INFO,"Core shutdown.");
    logger_shutdown();
}