#include <gtest/gtest.h>
#include <ww_memory.h>

using namespace wwlockfree;

class WwMemoryTest
    : public testing::Test
{
public:
    using int_allocator_type = allocator<int>;

    int_allocator_type alloc;   // int类型分配器
};

// 测试allocate和deallocate
TEST_F(WwMemoryTest, AllocateAndDeallocate)
{
    int * p = alloc.allocate(10);
    EXPECT_NE(p, nullptr);

    alloc.deallocate(p, 10);
}

// 测试construct和destroy
TEST_F(WwMemoryTest, ConstructAndDestroy)
{
    int * p = alloc.allocate(10);

    for (int i = 0; i < 10; ++i) {
        alloc.construct(p + i, i);
    }

    for (int i = 0; i < 10; ++i) {
        EXPECT_EQ(p[i], i);
    }

    for (int i = 0; i < 10; ++i) {
        alloc.destroy(p + i);
    }

    alloc.deallocate(p, 10);
}