#include "lockfree_hashmap.h"
#include <string.h>
#include <assert.h>
#include <atomic>

struct lockfree_hashmap_node
{
	lockfree_hashmap_node() : key(0), value(0) { }

	std::atomic<void*> key;
	std::atomic<void*> value;
};

struct lockfree_hashmap
{
	struct lockfree_hashmap_node* node_array;
	unsigned node_array_size;
	hash_function* hash_fn;
	std::atomic<unsigned> key_count;
	std::atomic<int> lock;
	std::atomic<int> ref;
};

extern "C" struct lockfree_hashmap* lockfree_hashmap_create(unsigned prealloc_size, hash_function* hash_fn)
{
	struct lockfree_hashmap* hashmap = new struct lockfree_hashmap;
	if (!hashmap)
		return NULL;

	hashmap->node_array = new struct lockfree_hashmap_node[prealloc_size];
	hashmap->node_array_size = prealloc_size;
	hashmap->hash_fn = hash_fn;
	hashmap->key_count.store(0);
	hashmap->lock.store(0);
	hashmap->ref.store(0);
	
	if (!hashmap->node_array) {
		lockfree_hashmap_destory(hashmap);
		return NULL;
	}
	return hashmap;
}

extern "C" void lockfree_hashmap_destory(struct lockfree_hashmap* hashmap)
{
	if (!hashmap)
		return;

	if (hashmap->node_array) {
		delete[] hashmap->node_array;
		hashmap->node_array = NULL;
	}
	delete hashmap;
}

static void* lockfree_hashmap_set_real(int* is_newkey, struct lockfree_hashmap_node* node_array, unsigned node_array_size,
	hash_function* hash_fn, void* key, void* value, int overwrite)
{
	for (unsigned idx = hash_fn(key); ; idx++) {
		idx %= node_array_size;

		struct lockfree_hashmap_node* node = &node_array[idx];

		void* exp_key = NULL;
		if (!node->key.compare_exchange_strong(exp_key, key)) {
			if (exp_key != key)
				continue;
		} else {
			*is_newkey = true;
		}

		void* exp_value = NULL;
		if (!node->value.compare_exchange_strong(exp_value, value)) {
			if (!overwrite)
				return exp_value;
		}

		if (!exp_value)
			return exp_value;

		assert(overwrite);

		while (!node->value.compare_exchange_strong(exp_value, value))
			;
		return exp_value;
	}
}

static int lockfree_hashmap_resize(struct lockfree_hashmap* hashmap)
{
	unsigned node_array_size = hashmap->node_array_size + (hashmap->node_array_size + 1) / 2;
	struct lockfree_hashmap_node* node_array = new struct lockfree_hashmap_node[node_array_size];
	if (!node_array)
		return false;

	for (unsigned i = 0; i < hashmap->node_array_size; i++) {
		struct lockfree_hashmap_node* node = &hashmap->node_array[i];
		void* key = node->key.load();
		void* value = node->value.load();
		int is_newkey = false;
		void* result = lockfree_hashmap_set_real(&is_newkey, node_array, node_array_size, hashmap->hash_fn, key, value, false);
		assert(result == NULL); (result);
		assert(is_newkey == true); (is_newkey);
	}

	delete[] hashmap->node_array;
	hashmap->node_array = node_array;
	hashmap->node_array_size = node_array_size;
	return true;
}

static int lockfree_hashmap_set_lock(struct lockfree_hashmap* hashmap)
{
	while (hashmap->lock.load())
		;

	hashmap->ref++;

	unsigned key_count = hashmap->key_count++;
	if (key_count >= hashmap->node_array_size) {
		int exp_lock = 0;
		if (hashmap->lock.compare_exchange_strong(exp_lock, 1)) {
			while (hashmap->ref.load() != 1)
				;
			
			lockfree_hashmap_resize(hashmap); // TODO alloc failed.
			hashmap->lock.store(0);
			return true;
		} else {
			hashmap->ref--;
			hashmap->key_count--;
			return lockfree_hashmap_set_lock(hashmap);
		}
	}
	return true;
}


extern "C" void* lockfree_hashmap_set(struct lockfree_hashmap* hashmap, void* key, void* value, int overwrite)
{
	assert(key);

	lockfree_hashmap_set_lock(hashmap);

	int is_newkey = false;
	void* result = lockfree_hashmap_set_real(&is_newkey, hashmap->node_array, hashmap->node_array_size, hashmap->hash_fn, key, value, overwrite);
	if (!is_newkey)
		hashmap->key_count--;
	hashmap->ref--;

	return result;
}

static void lockfree_hashmap_get_lock(struct lockfree_hashmap* hashmap)
{
	while (hashmap->lock.load())
		;

	hashmap->ref++;
	if (hashmap->lock.load()) {
		hashmap->ref--;
		return lockfree_hashmap_get_lock(hashmap);
	}
}

static void* lockfree_hashmap_get_real(struct lockfree_hashmap* hashmap, void* key)
{
	for (unsigned idx = hashmap->hash_fn(key); ; idx++) {
		idx %= hashmap->node_array_size;

		struct lockfree_hashmap_node* node = &hashmap->node_array[idx];

		void* cur_key = node->key.load();
		if (!cur_key)
			return NULL;

		if (cur_key == key)
			return node->value.load();
	}
}

extern "C" void* lockfree_hashmap_get(struct lockfree_hashmap* hashmap, void* key)
{
	lockfree_hashmap_get_lock(hashmap);

	void* result = lockfree_hashmap_get_real(hashmap, key);
	hashmap->ref--;

	return result;
}

extern "C" unsigned lockfree_hashmap_size(struct lockfree_hashmap* hashmap)
{
	return hashmap->key_count.load();
}