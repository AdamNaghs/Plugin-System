#ifndef _PLUGIN_API_H
#define _PLUGIN_API_H

#include "core_context.h"
typedef struct PluginMetadata {
    const char* name;
    const char** depends_on;  // NULL-terminated list
} PluginMetadata;

typedef struct PluginAPI {
    int (*init)(CoreContext*);
    int (*update)(CoreContext*);
    int (*shutdown)(CoreContext*);
    PluginMetadata* meta;  // NEW
} PluginAPI;

/* The user define this function to return the struct above */
PluginAPI Load();

#endif /* _PLUGIN_API_H */