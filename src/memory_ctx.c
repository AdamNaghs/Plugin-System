#include "../include/memory_ctx.h"
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void free_value_generic(void *value) {
    free(value);
}

MapTypeData map_type = MAP_TYPE(
    char*, void*,
    map_default_hash_str,
    map_default_cmp_str,
    map_default_free_str,
    free_value_generic
);

MapTypeData get_mem_ctx_map_type()
{
    return map_type;
}

void* mem_ctx_alloc(Map* map, const char* key, size_t value_size)
{
    assert(map != NULL);
    assert(key != NULL);
    assert(value_size > 0);

    char *heap_key = strdup(key);
    assert(heap_key != NULL);

    void *heap_value = malloc(value_size);
    assert(heap_value != NULL);
    memset(heap_value, 0, value_size);  // Optional: zero-init

    int ret = map_add(map, &heap_key, &heap_value);
    assert(ret >= 0);  // Allow overwrite

    return heap_value;
}

/* usage ensures that all data is on the heap */

void mem_ctx_set(Map* map, const char* key, void* value, size_t value_size) {
    assert(map != NULL);
    assert(key != NULL);
    assert(value != NULL);
    assert(value_size > 0);

    char *heap_key = strdup(key);
    assert(heap_key != NULL);

    void *heap_value = malloc(value_size);
    assert(heap_value != NULL);

    memcpy(heap_value, value, value_size);
    int ret = map_add(map, &heap_key, &heap_value);
    assert(ret >= 0);  // 0 = success, 1 = updated existing, -1 = failure
}



