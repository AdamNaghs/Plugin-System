/**
 * @file plugin.h
 * @brief Defines the plugin manager, which handles dynamic loading and execution of plugins.
 *
 * The PluginManager is responsible for loading shared plugin libraries,
 * resolving dependencies, and managing lifecycle events for all active plugins.
 */

 #ifndef _PLUGIN_MANAGER_H
 #define _PLUGIN_MANAGER_H
 
 #include "plugin_api.h"
 
 /**
  * @brief Represents a single loaded plugin.
  */
 typedef struct Plugin {
     PluginAPI* api;   /**< Pointer to the plugin's API (init, update, shutdown). */
     void* handle;     /**< Platform-specific handle to the loaded shared library. */
     char* name;       /**< Name of the plugin (typically filename or metadata name). */
     int version;      /**< Optional version tag (currently unused). */
 } Plugin;
 
 /**
  * @brief Manages loading, initialization, updates, and unloading of all plugins.
  */
 typedef struct PluginManager {
     struct {
         Plugin* list;   /**< Array of loaded plugins. */
         size_t len;     /**< Number of loaded plugins. */
     } plugins;
 
     CoreContext* ctx;  /**< Pointer to the global CoreContext shared across plugins. */
 } PluginManager;
 
 /**
  * @brief Initializes the plugin manager and loads plugins from the specified folder.
  *
  * @param pm            Pointer to the PluginManager.
  * @param folder_path   Path to the folder containing plugin `.so`/`.dll` files.
  */
 void plugin_manager_new(PluginManager* pm, char* folder_path);
 
 /**
  * @brief Frees all plugins and associated resources.
  *
  * @param pm Pointer to the PluginManager.
  */
 void plugin_manager_free(PluginManager* pm);

 /**
  * @brief Hot reloads all plugins.
  * 
  * @param pm   Pointer to the PluginManager.
  * @param ctx  Pointer to the CoreContext.
  */
 void plugin_manager_hot_reload(PluginManager* pm, CoreContext* ctx);
 
 /**
  * @brief Initializes all plugins, resolving and respecting dependencies.
  *
  * @param pm   Pointer to the PluginManager.
  * @param ctx  Pointer to the CoreContext.
  */
 void plugin_manager_init(PluginManager* pm, CoreContext* ctx);
 
 /**
  * @brief Calls the update function on each plugin.
  *
  * @param pm   Pointer to the PluginManager.
  * @param ctx  Pointer to the CoreContext.
  */
 void plugin_manager_update(PluginManager* pm, CoreContext* ctx);
 
 /**
  * @brief Calls the shutdown function on each plugin.
  *
  * @param pm   Pointer to the PluginManager.
  * @param ctx  Pointer to the CoreContext.
  */
 void plugin_manager_shutdown(PluginManager* pm, CoreContext* ctx);
 
 #endif /* _PLUGIN_MANAGER_H */
 