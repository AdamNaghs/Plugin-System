#ifndef _PLUGIN_API_H
#define _PLUGIN_API_H

#include "core_context.h"

typedef int (*Plugin_Function)(CoreContext*);

typedef struct PluginAPI
{
    int (*init)(CoreContext*);
    int (*update)(CoreContext*);
    int (*draw)(CoreContext*);
    int (*shutdown)(CoreContext*);
} PluginAPI;

/* The user define this function to return the struct above */
PluginAPI Load();

#endif /* _PLUGIN_API_H */