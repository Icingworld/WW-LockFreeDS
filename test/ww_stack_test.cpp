#include <thread>

#include <gtest/gtest.h>
#include <ww_stack.h>

using namespace wwlockfree;

class WWStackTest
    : public testing::Test
{
public:
    stack<int> m_stack;
};

TEST_F(WWStackTest, push)
{
    
}
