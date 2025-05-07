/**
 * @file plugin_api.h
 * @brief Defines the standardized interface for engine plugins.
 *
 * Plugins must implement the `Load()` function to return a `PluginAPI` structure,
 * which defines initialization, update, and shutdown behavior, along with metadata.
 */

 #ifndef _PLUGIN_API_H
 #define _PLUGIN_API_H
 
 #include "core_context.h"
 
 /**
  * @brief Metadata about a plugin, used for dependency resolution and identification.
  */
 typedef struct PluginMetadata {
     const char* name;          /**< Unique plugin name (e.g., "Scheduler", "Signals"). */
     const char** required_deps;   /**< NULL-terminated list of plugin names this plugin depends on. */
     const char** optional_deps;
 } PluginMetadata;
 
 /**
  * @brief Defines the lifecycle interface of a plugin.
  *
  * Each plugin must return a `PluginAPI` struct from `Load()`, describing
  * its initialization, update, and shutdown behavior.
  */
 typedef struct PluginAPI {
     /**
      * @brief Called once when the plugin is loaded.
      *
      * @param ctx Pointer to the shared CoreContext.
      * @return 0 on success, non-zero on failure.
      */
     int (*init)(CoreContext* ctx);
 
     /**
      * @brief Called once per frame or tick.
      *
      * @param ctx Pointer to the shared CoreContext.
      * @return 0 on success, non-zero to signal failure or exit.
      */
     int (*update)(CoreContext* ctx);
 
     /**
      * @brief Called once when the plugin is shutting down.
      *
      * @param ctx Pointer to the shared CoreContext.
      * @return 0 on success.
      */
     int (*shutdown)(CoreContext* ctx);
 
     /**
      * @brief Pointer to plugin metadata (name and dependencies).
      */
     PluginMetadata* meta;
 } PluginAPI;
 
 /**
  * @brief Entry point that must be defined by all plugins.
  *
  * This function returns a pointer to the static or allocated `PluginAPI` structure
  * defining the plugin's lifecycle and metadata.
  *
  * @return A fully populated PluginAPI instance.
  */
 PluginAPI Load();
 
 #endif /* _PLUGIN_API_H */
 