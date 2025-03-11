#pragma once

#include <atomic>
#include <memory>
#include <thread>

namespace wwlockfree
{

/**
 * @brief hazard_pointer_obj_base
 * @tparam _Ty 数据类型
 * @tparam _DelTy 删除器
 * @link https://cppreference.cn/w/cpp/header/hazard_pointer
 * @details 风险指针保护基类，一切想要被风险指针保护的类都需要继承这个类
 * 在目前的C++26标准中，D deleter(private)仅作展示，不实现
 */
template <
    typename _Ty,
    typename _DelTy = std::default_delete<_Ty>
> class hazard_pointer_obj_base
{
protected:
    hazard_pointer_obj_base() = default;

    hazard_pointer_obj_base(const hazard_pointer_obj_base &) = default;

    hazard_pointer_obj_base(hazard_pointer_obj_base &&) = default;

    hazard_pointer_obj_base & operator=(const hazard_pointer_obj_base &) = default;

    hazard_pointer_obj_base & operator=(hazard_pointer_obj_base &&) = default;

    ~hazard_pointer_obj_base() = default;

public:
    /**
     * @brief 释放该对象
     */
    void retire(_DelTy _Deleter = _DelTy()) noexcept
    {
        _Deleter(static_cast<T *>(this));
    }
};

/**
 * @brief hazard pointer
 * @link https://cppreference.cn/w/cpp/header/hazard_pointer
 */
class hazard_pointer
{
private:
    std::atomic<void *> * _Protect_ptr;         // 指向保护指针的指针
    std::atomic<std::thread::id> _Thread_id;    // 线程ID

public:
    hazard_pointer() noexcept
        : _Protect_ptr(::new std::atomic<void *>(nullptr))
    {
    }

    hazard_pointer(hazard_pointer && _Other) noexcept
        : _Protect_ptr(_Other._Protect_ptr)
    {
        _Other._Protect_ptr = ::new std::atomic<void *>(nullptr);
    }

    hazard_pointer & operator=(hazard_pointer && _Other) noexcept
    {
        if (this != &_Other)
        {
            reset_protection();
            std::swap(_Protect_ptr, _Other._Protect_ptr);
        }
        return *this;
    }

    ~hazard_pointer()
    {
        reset_protection();
        delete _Protect_ptr;
    }

public:
    /**
     * @brief 风险指针是否为空
     */
    bool empty() const noexcept
    {
        return _Protect_ptr == nullptr;
    }

    /**
     * @brief 保护一个指针
     * @return 保护的指针
     */
    template <typename _Ty>
    _Ty * protect(const std::atomic<_Ty *> & _Src) noexcept
    {
        static_assert(std::is_base_of_v<hazard_pointer_obj_base<_Ty>, _Ty>,
            "Protected type must inherit from hazard_pointer_obj_base");
        _Ty * _Ptr;
        do {
            _Ptr = _Src.load(std::memory_order_acquire);
            _Protect_ptr->store(reinterpret_cast<void *>(_Ptr), std::memory_order_seq_cst);
        } while (_Ptr != _Src.load(std::memory_order_seq_cst));
        return _Ptr;
    }

    /**
     * @brief 尝试保护一个指针
     * @tparam _Ty 数据类型
     * @param _Ptr 期望读取的指针
     * @param _Src 目标原子变量
     * @return 保护是否成功
     */
    template <typename _Ty>
    bool try_protect(_Ty * & _Ptr, const std::atomic<_Ty *> & _Src) noexcept
    {
        // 暂时不实现
        return false;
    }

    /**
     * @brief 解除对指针的保护
     */
    template <typename _Ty>
    void reset_protection(_Ty * _Ptr) noexcept
    {
        if (_Ptr == reinterpret_cast<_Ty *>(_Protect_ptr->load(std::memory_order_acquire))) {
            _Protect_ptr->store(nullptr, std::memory_order_release);
        }
    }

    /**
     * @brief 重置风险指针
     */
    void reset_protection(std::nullptr_t = nullptr) noexcept
    {
        _Protect_ptr->store(nullptr, std::memory_order_seq_cst);
    }

    /**
     * @brief 交换风险指针
     */
    void swap(hazard_pointer & _Other) noexcept
    {
        std::swap(_Protect_ptr, _Other._Protect_ptr);
    }
};

/**
 * @brief 交换风险指针
 */
void swap(hazard_pointer & _Left, hazard_pointer & _Right) noexcept
{
    _Left.swap(_Right);
}

// 风险指针列表

static constexpr std::size_t _Hazard_pointer_max = 128;             // 风险指针最大数量
static hazard_pointer _Hazard_pointer_list[_Hazard_pointer_max];    // 风险指针列表

} // namespace wwlockfree
