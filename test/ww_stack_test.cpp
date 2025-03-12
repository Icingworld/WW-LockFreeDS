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
    const int num_threads = 4;
    const int num_iterations = 1000;
    std::vector<std::thread> threads;

    for (int t = 0; t < num_threads; ++t) {
        threads.emplace_back([&]() {
            for (int i = 0; i < num_iterations; ++i) {
                m_stack.push(i);
            }
        });
    }

    for (auto & th : threads) {
        th.join();
    }
}
