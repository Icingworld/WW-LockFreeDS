#pragma once

#include <cstddef>
#include <mutex>

namespace wwlockfree
{

/**
 * @brief lock free allocator
 * @tparam T 数据类型
 * 
 * @details 
 * 此处简单使用mutex加锁，作为过渡的线程安全分配器
 * 后续将更换为带内存池、无锁的内存分配策略
 */
template <typename T>
class allocator
{
private:
    std::mutex _Mutex;      // 互斥锁

public:
    using value_type = T;
    using pointer = T*;
    using const_pointer = const T*;
    using reference = T&;
    using const_reference = const T&;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    template <typename U>
    class rebind
    {
    public:
        using other = allocator<U>;
    };

public:
    allocator() noexcept = default;

    allocator(const allocator & other) noexcept = default;
    
    template <typename U>
    allocator(const allocator<U> & other) noexcept 
    {
    };

    ~allocator() = default;

public:
    /**
     * @brief 分配n个元素的内存
     * @param n 元素个数
     * @param hint 会在hint附近分配内存，忽略
     * @return pointer 内存指针
     * @exception std::bad_array_new_length 超出最大尺寸
     * @exception std::bad_alloc 内存分配失败
     */
    pointer allocate(size_type n, const void *)
    {
        std::lock_guard<std::mutex> lock(_Mutex);

        if (n == 0)
            return nullptr;

        return static_cast<pointer>(::operator new(n * sizeof(value_type)));
    }

    /**
     * @brief 释放由allocate分配的内存
     * @param ptr 要释放的内存指针
     * @param n 元素个数，忽略
     */
    void deallocate(pointer ptr, size_type)
    {
        std::lock_guard<std::mutex> lock(_Mutex);

        if (ptr == nullptr)
            return;
        
        ::operator delete(ptr);
    }

    /**
     * @brief 构造对象
     * @tparam U 要构造的类型
     * @tparam Args 构造函数参数
     * @param ptr 要构造的内存指针
     * @param args 构造函数参数包
     */
    template <typename U, typename... Args>
    void construct(U * ptr, Args&&... args)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        ::new(ptr) U(std::forward<Args>(args)...);
    }

    /**
     * @brief 销毁对象
     * @tparam U 要销毁的类型
     * @param ptr 要销毁的内存指针
     */
    template <typename U>
    void destroy(U * ptr)
    {
        std::lock_guard<std::mutex> lock(_Mutex);
        ptr->~U();
    }
};

} // namespace wwlockfree
