#include "memory_map.h"

#define MM_LOAD_THRESHOLD 0.75f

bool mm_str_eq(String a, String b)
{
    return a.len == b.len && memcmp(a.data, b.data, a.len) == 0;
}

size_t mm_hash_default(const char *data, size_t len)
{
    size_t hash = 5381;
    for (size_t i = 0; i < len; ++i)
        hash = ((hash << 5) + hash) + (unsigned char)data[i]; // djb2
    return hash;
}

void mm_init(MemoryMap *mm, size_t buckets, malloc_fn_t _malloc, free_fn_t _free, hash_fn_t _hash)
{
    mm->capacity = buckets;
    mm->count = 0;
    mm->_malloc = _malloc;
    mm->_free = _free;
    mm->_hash = _hash;
    mm->buckets = _malloc(sizeof(MemoryBucket) * buckets);
}

void mm_check_optimize(MemoryMap *mm)
{
    if ((float)mm->count / mm->capacity > MM_LOAD_THRESHOLD)
        mm_optimize(mm, MM_LOAD_THRESHOLD);
}

void *mm_alloc(MemoryMap *mm, String name, size_t size)
{
    mm_check_optimize(mm);
    void *memory = mm->_malloc(size);
    if (!memory)
        return NULL;
    memset(memory, 0, size);
    mm_bind(mm, name, memory, size, true);
    return memory;
}

void mm_bind(MemoryMap *mm, String name, void *memory, size_t size, bool owned)
{
    
    size_t index = mm->_hash(name.data, name.len) % mm->capacity;
    MemoryBucket *bucket = &mm->buckets[index];

    // First time init for this bucket
    if (bucket->entries == NULL)
    {
        bucket->capacity = 4;
        bucket->entries = mm->_malloc(sizeof(MemoryEntry) * bucket->capacity);
        bucket->count = 0;
    }

    // Check if name already exists and update it
    for (size_t i = 0; i < bucket->count; ++i)
    {
        String existing = {bucket->entries[i].name, strlen(bucket->entries[i].name)};
        if (mm_str_eq(name, existing))
        {
            bucket->entries[i].data = memory;
            bucket->entries[i].size = size;
            bucket->entries[i].owned = owned;
            return;
        }
    }

    mm_check_optimize(mm);

    // Insert new entry
    MemoryEntry *entry = &bucket->entries[bucket->count++];
    entry->name = mm->_malloc(name.len + 1);
    memcpy(entry->name, name.data, name.len);
    entry->name[name.len] = '\0';
    entry->data = memory;
    entry->size = size;
    entry->owned = owned;
    mm->count++;
}

void *mm_get(MemoryMap *mm, String name)
{
    mm_check_optimize(mm);
    size_t index = mm->_hash(name.data, name.len) % mm->capacity;
    MemoryBucket *bucket = &mm->buckets[index];
    if (!bucket->entries)
        return NULL;

    for (size_t i = 0; i < bucket->count; ++i)
    {
        String existing = {bucket->entries[i].name, strlen(bucket->entries[i].name)};
        if (mm_str_eq(name, existing))
        {
            return bucket->entries[i].data;
        }
    }
    return NULL;
}

size_t mm_get_size(MemoryMap *mm, String name)
{
    size_t index = mm->_hash(name.data, name.len) % mm->capacity;
    MemoryBucket *bucket = &mm->buckets[index];
    if (!bucket->entries)
        return 0;

    for (size_t i = 0; i < bucket->count; ++i)
    {
        String existing = {bucket->entries[i].name, strlen(bucket->entries[i].name)};
        if (mm_str_eq(name, existing))
        {
            return bucket->entries[i].size;
        }
    }
    return 0;
}

void mm_free(MemoryMap *mm)
{
    for (size_t i = 0; i < mm->capacity; ++i)
    {
        MemoryBucket *bucket = &mm->buckets[i];
        for (size_t j = 0; j < bucket->count; ++j)
        {
            MemoryEntry *entry = &bucket->entries[j];
            if (entry->owned && entry->data)
            {
                mm->_free(entry->data);
            }
            mm->_free(entry->name);
        }
        mm->_free(bucket->entries);
    }
    mm->count = 0;
    mm->_free(mm->buckets);
    mm->buckets = NULL;
}

void mm_optimize(MemoryMap *mm, float target_load)
{
    size_t new_capacity = (size_t)((float)mm->count / target_load);
    if (new_capacity < 8)
        new_capacity = 8;

    MemoryBucket *new_buckets = mm->_malloc(sizeof(MemoryBucket) * new_capacity);
    if (!new_buckets)
        return;
    
    for (size_t i = 0; i < new_capacity; ++i)
    {
        new_buckets[i].entries = NULL;
        new_buckets[i].count = 0;
        new_buckets[i].capacity = 0;
    }

    for (size_t i = 0; i < mm->capacity; ++i)
    {
        MemoryBucket *old_bucket = &mm->buckets[i];
        for (size_t j = 0; j < old_bucket->count; ++j)
        {
            MemoryEntry *entry = &old_bucket->entries[j];
            size_t new_index = mm->_hash(entry->name, strlen(entry->name)) % new_capacity;
            MemoryBucket *new_bucket = &new_buckets[new_index];

            if (new_bucket->entries == NULL) 
            {
                new_bucket->capacity = 4;
                new_bucket->entries = mm->_malloc(sizeof(MemoryEntry) * new_bucket->capacity);
                new_bucket->count = 0;
            }

            if (new_bucket->count >= new_bucket->capacity)
            {
                new_bucket->capacity *= 2;
                new_bucket->entries = realloc(new_bucket->entries, sizeof(MemoryEntry) * new_bucket->capacity);
            }

            new_bucket->entries[new_bucket->count++] = *entry;
        }

        mm->_free(old_bucket->entries);
    }

    mm->_free(mm->buckets); //  free old instance
    mm->buckets = new_buckets; // connect new data
    mm->capacity = new_capacity;
}

int mm_remove(MemoryMap *mm, String name)
{
    mm_check_optimize(mm);
    size_t index = mm->_hash(name.data, name.len) % mm->capacity;
    MemoryBucket *bucket = &mm->buckets[index];
    if (!bucket->entries)
        return 0; // Entry not found

    for (size_t i = 0; i < bucket->count; ++i)
    {
        String existing = {bucket->entries[i].name, strlen(bucket->entries[i].name)};
        if (mm_str_eq(name, existing))
        {
            MemoryEntry *entry = &bucket->entries[i];

            // Free owned data
            if (entry->owned && entry->data)
            {
                mm->_free(entry->data);
            }
            mm->_free(entry->name);

            // Shift entries to fill the gap
            for (size_t j = i; j < bucket->count - 1; ++j)
            {
                bucket->entries[j] = bucket->entries[j + 1];
            }
            bucket->count--;
            mm->count--;

            // Shrink the bucket array if it's too sparse
            if (bucket->count == 0)
            {
                mm->_free(bucket->entries);
                bucket->entries = NULL;
                bucket->capacity = 0;
            }

            return 1; // Successfully removed
        }
    }
    return 0; // Entry not found
}