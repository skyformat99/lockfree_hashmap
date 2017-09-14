#include "gtest/gtest.h"
#include "lockfree_hashmap.h"
#include <assert.h>
#include <thread>

static unsigned hash(void* p)
{
	return (unsigned)p;
}

static void test_thread(struct lockfree_hashmap* hashmap, int index)
{
	std::this_thread::sleep_for(std::chrono::microseconds(5));

	for (int i = index + 1; i < index + 512; i++)
	{
		EXPECT_EQ(NULL, lockfree_hashmap_set(hashmap, (void*)i, (void*)-1, false));
		EXPECT_EQ((void*)-1, lockfree_hashmap_set(hashmap, (void*)i, (void*)i, true));
		EXPECT_EQ((void*)i, lockfree_hashmap_set(hashmap, (void*)i, (void*)-1, false));
		EXPECT_EQ((void*)i, lockfree_hashmap_get(hashmap, (void*)i));
	}
}

TEST(lockfree_hashmap, set)
{
	struct lockfree_hashmap* hashmap = lockfree_hashmap_create(16, hash);
	std::thread threads[16];
	for (size_t i = 0; i < _countof(threads); i++)
	{
		threads[i] = std::thread(test_thread, hashmap, 512 * i);
	}

	for (size_t i = 0; i < _countof(threads); i++)
	{
		threads[i].join();
	}
	lockfree_hashmap_destory(hashmap);
}

