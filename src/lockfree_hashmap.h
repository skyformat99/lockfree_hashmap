#pragma once

#ifdef __cplusplus
extern "C" {
#endif

typedef unsigned (hash_function)(void* key);
struct lockfree_hashmap* lockfree_hashmap_create(unsigned prealloc_size, hash_function* hash_fn);
void lockfree_hashmap_destory(struct lockfree_hashmap* hashmap);

void* lockfree_hashmap_set(struct lockfree_hashmap* hashmap, void* key, void* value, int overwrite);
void* lockfree_hashmap_get(struct lockfree_hashmap* hashmap, void* key);
unsigned lockfree_hashmap_size(struct lockfree_hashmap* hashmap);

#ifdef __cplusplus
}
#endif