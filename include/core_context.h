/**
 * @file core_context.h
 * @brief Defines the CoreContext used for plugin communication and memory access.
 *
 * CoreContext is passed to all plugins and acts as the shared execution and memory environment.
 * It exposes plugin-safe memory allocation, dynamic named storage, and timing values.
 */

 #ifndef _CORE_CONTEXT_H
 #define _CORE_CONTEXT_H
 
 #include "memory_map.h"
 
 /**
  * @def DEFAULT_MEMORY_BUCKETS
  * @brief Default number of hash buckets used in the internal memory map.
  */
 #define DEFAULT_MEMORY_BUCKETS 256
 
 /**
  * @def CC_ALLOC
  * @brief Allocates memory from the CoreContext's memory map with a string name.
  */
 #define CC_ALLOC(ctx,string,size) \
     (ctx)->memory.alloc(&(ctx)->memory.map, LIT(string), size)
 
 /**
  * @def CC_FREE
  * @brief Frees memory associated with the given name from the CoreContext's memory map.
  */
 #define CC_FREE(ctx,string) \
     (ctx)->memory.free(&(ctx)->memory.map,LIT(string))
 
 /**
  * @def CC_GET
  * @brief Retrieves a pointer to memory stored under the given name in the memory map.
  */
 #define CC_GET(ctx,string) \
     (ctx)->memory.get(&(ctx)->memory.map,LIT(string))
 
 /**
  * @def CC_BIND
  * @brief Binds an external pointer to a string name in the CoreContext's memory map.
  *
  * @param ctx     The CoreContext pointer.
  * @param string  String key for the memory binding.
  * @param data    Pointer to the memory.
  * @param size    Size of the memory.
  * @param owned   Whether the memory should be freed by the CoreContext.
  */
 #define CC_BIND(ctx,string,data,size,owned) \
     (ctx)->memory.bind(&(ctx)->memory.map,LIT(string),data,size,owned)
 
 /**
  * @brief The shared context passed to all plugins.
  *
  * Contains versioning, memory access API, and timing data.
  */
 typedef struct CoreContext
 {
     int version;  /**< The engine or core system version number. */
 
     struct
     {
         MemoryMap map;  /**< Internal memory map for named allocations. */
 
         /**
          * @brief Allocates a new memory region associated with a string key.
          */
         void* (*alloc)(MemoryMap*, String, size_t);
 
         /**
          * @brief Frees memory associated with a string key.
          */
         int (*free)(MemoryMap*, String);
 
         /**
          * @brief Retrieves a pointer from the memory map.
          */
         void* (*get)(MemoryMap*, String);
 
         /**
          * @brief Binds external memory to the memory map under a key.
          */
         void (*bind)(MemoryMap *mm, String name, void *memory, size_t size, bool owned);
     } memory;
 
     float delta_time;        /**< Time in seconds since the last frame. */
     float fixed_delta_time;  /**< Fixed time step, if used (optional). */
 } CoreContext;
 
 /**
  * @brief Initializes a CoreContext, setting up memory map and API pointers.
  *
  * @param ctx      Pointer to the CoreContext to initialize.
  * @param version  Optional version number for compatibility tracking.
  */
 void core_context_new(CoreContext* ctx, int version);
 
 /**
  * @brief Frees all memory and internal state from a CoreContext.
  *
  * @param ctx  Pointer to the CoreContext to free.
  */
 void core_context_free(CoreContext* ctx);
 
 /**
  * @brief Updates timing data such as delta time in the CoreContext.
  *
  * Typically called once per frame.
  *
  * @param ctx  Pointer to the CoreContext to update.
  */
 void core_context_update(CoreContext* ctx);
 
 #endif /* _CORE_CONTEXT_H */
 