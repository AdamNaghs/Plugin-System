#ifndef _CORE_H
#define _CORE_H

#include "../include/plugin.h"


typedef struct Core
{
    int should_run;
    PluginManager manager;
    CoreContext context;
} Core;

void core_init(Core* core,int version, char* plugin_folder);

void core_run(Core* core);

void core_shutdown(Core* core);

#endif /*_CORE_H*/