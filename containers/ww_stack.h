#pragma once

#include <atomic>

#include "ww_memory.h"

namespace wwlockfree
{

/**
 * @brief lock free stack
 * @tparam T 数据类型
 * @tparam Allocator 分配器
 */
template <
    typename T,
    typename Allocator = wwlockfree::allocator<T>
> class stack
{

};

} // namespace wwlockfree
