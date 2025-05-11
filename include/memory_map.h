/**
 * @file memory_map.h
 * @brief Provides a string-keyed memory registry for runtime allocations and binding.
 *
 * This system implements a lightweight hash map where memory can be allocated,
 * retrieved, or externally bound to string keys. Useful for plugin contexts,
 * dynamic state sharing, and modular systems.
 */

 #ifndef _MEMORY_MAP_H
 #define _MEMORY_MAP_H
 
 #include <stdlib.h>
 #include <stdbool.h>
 #include <string.h>
 
 /**
  * @brief Represents a lightweight, immutable string used as a key.
  */
 typedef struct String {
     char* data;     /**< Pointer to the string data. */
     size_t len;     /**< Length of the string (excluding null terminator). */
 } String;
 
 /**
  * @def STR(s)
  * @brief Converts a null-terminated C string into a String object.
  */
 #define STR(s) ((String){(s), strlen(s)})
 
 /**
  * @def LIT(str)
  * @brief Converts a string literal into a String object (length computed at compile time).
  */
 #define LIT(str) ((String){str,sizeof(str)-1})
 
 /**
  * @brief Compares two String objects for equality.
  *
  * @param a First string.
  * @param b Second string.
  * @return true if equal, false otherwise.
  */
 bool mm_str_eq(String a, String b);
 
 /**
  * @brief Default hash function using djb2 algorithm.
  *
  * @param data Pointer to string data.
  * @param len Length of the string.
  * @return Computed hash value.
  */
 size_t mm_hash_default(const char *data, size_t len);
 
 /**
  * @brief Represents an entry in a memory bucket (key-value pair).
  */
 typedef struct MemoryEntry {
     char *name;     /**< Allocated copy of the string key. */
     void *data;     /**< Pointer to the memory block. */
     size_t size;    /**< Size of the memory block. */
     bool owned;     /**< Whether the memory should be freed by the map. */
 } MemoryEntry;
 
 /**
  * @brief Represents a bucket that holds multiple MemoryEntry items.
  */
 typedef struct MemoryBucket {
     MemoryEntry *entries;  /**< Dynamic array of entries. */
     size_t count;          /**< Number of entries in use. */
     size_t capacity;       /**< Allocated capacity of the entry array. */
 } MemoryBucket;
 
 /**
  * @brief Represents the hash map itself, made up of buckets.
  */
 typedef struct MemoryMap {
     MemoryBucket *buckets;      /**< Array of buckets. */
     size_t count;               /**< Total number of key-value entries stored. */
     size_t capacity;            /**< Number of buckets in the hash map. */
     void *(*_malloc)(size_t);   /**< Memory allocator function. */
     void (*_free)(void *);      /**< Memory deallocator function. */
     size_t (*_hash)(const char *, size_t);  /**< Hash function for string keys. */
 } MemoryMap;
 
 /**
  * @brief Function pointer type for memory allocation.
  */
 typedef void *(*malloc_fn_t)(size_t);
 
 /**
  * @brief Function pointer type for memory deallocation.
  */
 typedef void (*free_fn_t)(void *);
 
 /**
  * @brief Function pointer type for hash functions.
  */
 typedef size_t (*hash_fn_t)(const char *, size_t);
 
 /**
  * @brief Initializes a MemoryMap with the given bucket count and function pointers.
  *
  * @param mm Pointer to the MemoryMap to initialize.
  * @param buckets Number of hash buckets.
  * @param _malloc Memory allocation function.
  * @param _free Memory free function.
  * @param _hash Hashing function to use for keys.
  */
 void mm_init(MemoryMap *mm, size_t buckets, malloc_fn_t _malloc, free_fn_t _free, hash_fn_t _hash);
 
 /**
  * @brief Allocates memory from the map and binds it to a string key.
  *
  * @param mm Pointer to the MemoryMap.
  * @param name Key under which the memory is stored.
  * @param size Size of memory to allocate.
  * @return Pointer to the allocated memory.
  */
 void *mm_alloc(MemoryMap *mm, String name, size_t size);
 
 /**
  * @brief Retrieves memory by key.
  *
  * @param mm Pointer to the MemoryMap.
  * @param name Key to look up.
  * @return Pointer to memory, or NULL if not found.
  */
 void *mm_get(MemoryMap *mm, String name);

  /**
  * @brief Retrieves memory by key.
  *
  * @param mm Pointer to the MemoryMap.
  * @param name Key to look up.
  * @return Size of value at key. 0 if key is not found.
  */ 
  size_t mm_get_size(MemoryMap *mm, String name);
 
 /**
  * @brief Binds an external memory pointer to a string key.
  *
  * @param mm Pointer to the MemoryMap.
  * @param name Key under which to bind.
  * @param memory Pointer to the memory to bind.
  * @param size Size of the memory.
  * @param owned If true, the map will free the memory on cleanup.
  */
 void mm_bind(MemoryMap *mm, String name, void *memory, size_t size, bool owned);
 
 /**
  * @brief Frees all memory in the map, including owned entries.
  *
  * @param mm Pointer to the MemoryMap.
  */
 void mm_free(MemoryMap *mm);
 
 /**
  * @brief Rehashes and expands the map to meet a target load factor.
  *
  * @param mm Pointer to the MemoryMap.
  * @param target_load Desired load factor (e.g. 0.75).
  */
 void mm_optimize(MemoryMap *mm, float target_load);
 
 /**
  * @brief Removes an entry by key.
  *
  * @param mm Pointer to the MemoryMap.
  * @param name Key to remove.
  * @return 1 if removed, 0 if not found.
  */
 int mm_remove(MemoryMap* mm, String name);
 
 #endif /* _MEMORY_MAP_H */
 