#ifndef _MEM_CTX_H
#define _MEM_CTX_H

#include "../external/C-Collection-Map/map.h"

/* These functions are implemented because the map versions are not available or need to be mapped for this context*/

void* mem_ctx_alloc(Map*, const char*, size_t);

void mem_ctx_set(Map*, const char* key,void* value,size_t value_size);

#define MEM_CTX_SET_VALUE(map,key,value) mem_ctx_set(map,key,&value,sizeof(value));

#define MEM_CTX_SET_PTR(map, key, ptr, size) mem_ctx_set(map, key, ptr, size)

MapTypeData get_mem_ctx_map_type();

#endif /*_MEM_CTX_H*/