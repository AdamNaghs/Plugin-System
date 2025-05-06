/**
 * @file core.h
 * @brief Defines the Core struct and functions that drive plugin lifecycle and execution.
 *
 * The Core object is the central runtime structure that owns the plugin manager and execution context.
 * It is responsible for initializing, running, and shutting down the entire system.
 */

 #ifndef _CORE_H
 #define _CORE_H
 
 #include "../include/plugin.h"
 
 /**
  * @brief Represents the central execution state of the system.
  *
  * Contains both the plugin manager and the shared CoreContext, and is passed to the main program loop.
  */
 typedef struct Core
 {
     PluginManager manager;  /**< Plugin manager responsible for loading and running plugins. */
     CoreContext context;    /**< Shared runtime context for memory, timing, and state. */
 } Core;
 
 /**
  * @brief Initializes the core system, context, and plugin manager.
  *
  * @param core           Pointer to the Core instance to initialize.
  * @param version        Version number of the engine/core system.
  * @param plugin_folder  Path to the folder containing plugin shared libraries.
  */
 void core_init(Core* core, int version, char* plugin_folder);
 
 /**
  * @brief Starts the main loop and calls plugin updates until the system requests shutdown.
  *
  * @param core Pointer to the initialized Core instance.
  */
 void core_run(Core* core);
 
 /**
  * @brief Gracefully shuts down all plugins and frees associated resources.
  *
  * @param core Pointer to the Core instance to shut down.
  */
 void core_shutdown(Core* core);
 
 #endif /* _CORE_H */
 