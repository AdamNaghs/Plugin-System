#ifndef _MEMORY_MAP_H
#define _MEMORY_MAP_H

#include <stdlib.h>
#include <stdbool.h>
#include <string.h>

typedef struct String
{
    char* data;
    size_t len;
} String;

#define STR(s) ((String){(s), strlen(s)})
#define LIT(str) ((String){str,sizeof(str)-1})

bool mm_str_eq(String a, String b);

size_t mm_hash_default(const char *data, size_t len);

typedef struct MemoryEntry
{
    char *name;
    void *data;
    size_t size;
    bool owned;
} MemoryEntry;

typedef struct MemoryBucket
{
    MemoryEntry *entries;
    size_t count;
    size_t capacity;
} MemoryBucket;

typedef struct MemoryMap
{
    MemoryBucket *buckets;
    size_t count;
    size_t capacity;
    void *(*_malloc)(size_t);
    void (*_free)(void *);
    size_t (*_hash)(const char *, size_t);
} MemoryMap;

typedef void *(*malloc_fn_t)(size_t);
typedef void (*free_fn_t)(void *);
typedef size_t (*hash_fn_t)(const char *, size_t);

void mm_init(MemoryMap *mm, size_t buckets, malloc_fn_t _malloc, free_fn_t _free, hash_fn_t _hash);

void *mm_alloc(MemoryMap *mm, String name, size_t size);

void *mm_get(MemoryMap *mm, String name);

/* If owned is true then the memory map will free the data too. Otherwise this can be used to reference memory by name. */
void mm_bind(MemoryMap *mm, String name, void *memory, size_t size, bool owned);

/* doesnt free memory that isnt owned */
void mm_free(MemoryMap *mm);

/* 0.75 is usually considered optimal */
void mm_optimize(MemoryMap *mm, float target_load);

/* 1 on success 0, on fail*/
int mm_remove(MemoryMap* mm, String name);

#endif