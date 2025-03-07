#pragma once

#include <atomic>
#include <memory>

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
    static constexpr unsigned int _Max_hazard_pointers = 128;               // 最大风险指针数量
    static thread_local hazard_pointer _Current_hazard_pointer;             // 当前线程的风险指针
    static hazard_pointer _Global_hazard_pointer[_Max_hazard_pointers];     // 全局风险指针数组

    void * _Protect_ptr;                                                    // 保护的指针

public:
    hazard_pointer() noexcept
        : _Protect_ptr(nullptr)
    {
    }

    hazard_pointer(hazard_pointer && _Other) noexcept
        : _Protect_ptr(_Other._Protect_ptr)
    {
        _Other._Protect_ptr = nullptr;
    }

    hazard_pointer & operator=(hazard_pointer && _Other) noexcept
    {
        if (this != &_Other)
        {
            reset_protection();
            _Protect_ptr = _Other._Protect_ptr;
            _Other._Protect_ptr = nullptr;
        }
        return *this;
    }

    ~hazard_pointer()
    {
        reset_protection();
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
        _Ty * _Ptr;
        do {
            _Ptr = _Src.load();
            _Protect_ptr = _Ptr;
        } while (_Protect_ptr != _Src.load());
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
        _Ty * _Expected = _Ptr;
        _Ptr = _Src.load();
        if (_Ptr == _Expected) {
            _Protect_ptr = _Ptr;
            return true;
        }
        return false;
    }

    /**
     * @brief 解除对指针的保护
     */
    template <typename _Ty>
    void reset_protection(_Ty * _Ptr) noexcept
    {
        if (_Protect_ptr == _Ptr) {
            _Protect_ptr = nullptr;
        }
    }

    /**
     * @brief 重置风险指针
     */
    void reset_protection(std::nullptr_t = nullptr) noexcept
    {
        _Protect_ptr = nullptr;
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

} // namespace wwlockfree
