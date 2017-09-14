#include "gtest/gtest.h"
#include "lockfree_hashmap.h"
#include <assert.h>
#include <thread>
#include <mutex>

static unsigned hash(void* p)
{
	return (unsigned)p;
}

static void test_thread(struct lockfree_hashmap* hashmap, int index)
{
	std::this_thread::sleep_for(std::chrono::microseconds(5));

	for (int i = index + 1; i < index + 512; i++) {
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
	for (size_t i = 0; i < _countof(threads); i++)  {
		threads[i] = std::thread(test_thread, hashmap, 512 * i);
	}

	for (size_t i = 0; i < _countof(threads); i++) {
		threads[i].join();
	}
	ASSERT_EQ(16 * 512 - 16, lockfree_hashmap_size(hashmap));
	lockfree_hashmap_destory(hashmap);
}

TEST(lockfree_hashmap, set_0)
{
	struct lockfree_hashmap* hashmap = lockfree_hashmap_create(16, hash);
	for (int i = 1; i <= 512; i++) {
		EXPECT_EQ(NULL, lockfree_hashmap_set(hashmap, (void*)i, (void*)-1, false));
		EXPECT_EQ((void*)-1, lockfree_hashmap_set(hashmap, (void*)i, NULL, true));
		EXPECT_EQ(NULL, lockfree_hashmap_set(hashmap, (void*)i, (void*)-1, false));
		EXPECT_EQ((void*)-1, lockfree_hashmap_set(hashmap, (void*)i, NULL, true));
		EXPECT_EQ(NULL, lockfree_hashmap_get(hashmap, (void*)i));
	}
	ASSERT_EQ(512, lockfree_hashmap_size(hashmap));
	lockfree_hashmap_destory(hashmap);
}