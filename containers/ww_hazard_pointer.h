#pragma once

#include <atomic>
#include <memory>
#include <thread>
#include <functional>

namespace wwlockfree
{

class hazard_pointer;

// 风险指针列表

static constexpr std::size_t _Hazard_pointer_max = 128;             // 风险指针最大数量
static hazard_pointer _Hazard_pointer_list[_Hazard_pointer_max];    // 风险指针列表

/**
 * @brief 检查一个指针是否被风险指针保护
 * @param _Ptr 指针
 * @return 是否被风险指针保护
 */
bool _Search_hazard_pointer(void * _Ptr)
{
    for (std::size_t i = 0; i < _Hazard_pointer_max; ++i) {
        if (_Hazard_pointer_list[i]._Protect_ptr->load() == _Ptr) {
            return true;
        }
    }
    return false;
}

/**
 * @brief hazard pointer reclaim node
 * @details 用于延迟删除的链表节点
 */
class _Relaim_node
{
public:
    using _Node_pointer = _Relaim_node *;

public:
    _Node_pointer _Next;                    // 后继指针
    void * _Del_ptr;                        // 待删除指针
    std::function<void(void*)> _Deleter;    // 删除器

public:
    template<typename _Ty>
    _Relaim_node(_Ty * _Ptr)
        : _Next(nullptr)
        , _Del_ptr(_Ptr)
       , _Deleter([](void * _P) {
            delete static_cast<_Ty *>(_P);
        })
    {
    }

    ~_Relaim_node()
    {
        _Deleter(_Del_ptr);
    }
};

/**
 * @brief hazard pointer reclaim list
 * @details 用于延迟删除的链表
 */
class _Reclaim_list
{
public:
    using _Node = _Relaim_node;
    using _Node_pointer = _Relaim_node *;

public:
    std::atomic<_Node_pointer> _Head;   // 头指针

public:
    _Reclaim_list()
    {
        _Head.store(nullptr);
    }

public:
    /**
     * @brief 插入一个节点
     */
    void push(const _Node_pointer & _Ptr)
    {
        // 直接插入到链表头部
        _Ptr->_Next = _Head.load();
        while (!_Head.compare_exchange_weak(_Ptr->_Next, _Ptr));
    }

    /**
     * @brief 将一个指针插入到链表中
     */
    template <typename _Ty>
    void push(_Ty * _Ptr)
    {
        push(new _Node(_Ptr));
    }

    /**
     * @brief 释放所有节点
     * @details 尝试释放所有能够释放的节点
     * 不能释放的节点会重新被添加到链表中
     */
    void release() {
        // 获取当前所有待删除的节点，并清空 _Head
        _Node_pointer current = _Head.exchange(nullptr);
    
        while (current) {
            _Node_pointer next = current->_Next;
    
            // 检查当前节点是否仍然被风险指针保护
            if (!_Search_hazard_pointer(current->_Del_ptr)) {
                delete current;  // 没有风险指针，直接删除
            } else {
                push(current);  // 仍然有风险指针，重新放入待删除链表
            }
    
            current = next;
        }
    }
};

static _Reclaim_list _Reclaim_list_instance;    // 延迟删除链表

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
    bool _Should_delay;    // 是否延迟删除

protected:
    hazard_pointer_obj_base()
        : _Should_delay(false)
    {
    }

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
        if (_Should_delay) {
            // 延迟删除，添加到待删除链表中
            _Reclaim_list_instance.push(static_cast<_Ty *>(this));
        } else {
            // 立即删除
            _Deleter(static_cast<_Ty *>(this));
        }
    }
};

/**
 * @brief hazard pointer
 * @link https://cppreference.cn/w/cpp/header/hazard_pointer
 */
class hazard_pointer
{
public:
    std::atomic<void *> * _Protect_ptr;         // 指向保护指针的指针
    std::atomic<std::thread::id> _Thread_id;    // 线程ID

public:
    hazard_pointer() noexcept
        : _Protect_ptr(::new std::atomic<void *>(nullptr))
        , _Thread_id()
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

} // namespace wwlockfree
