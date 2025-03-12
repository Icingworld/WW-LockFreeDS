#pragma once

#include "ww_memory.h"
#include "ww_hazard_pointer.h"

namespace wwlockfree
{

/**
 * @brief stack node
 * @tparam _Ty 数据类型
 */
template <typename _Ty>
class _Stack_node
    : public hazard_pointer_obj_base<_Stack_node<_Ty>>
{
public:
    using value_type = _Ty;
    using _Node_pointer = _Stack_node<_Ty> *;

public:
    value_type _Value;          // 数据
    _Node_pointer _Next;        // 后继指针

public:
    _Stack_node(const value_type & _New_value)
        : _Should_delay(false)
        , _Value(_New_value)
        , _Next(nullptr)
    {
    }

    ~_Stack_node()
    {
        retire();
    }
};

/**
 * @brief lock free stack
 * @tparam _Ty 数据类型
 * @tparam _AllocTy 分配器
 */
template <
    typename _Ty,
    typename _AllocTy = wwlockfree::allocator<_Ty>
> class stack
{
public:
    using value_type = _Ty;
    using allocator_type = _AllocTy;

    using _Node = _Stack_node<_Ty>;
    using _Node_pointer = _Stack_node<_Ty> *;

public:
    std::atomic<_Node_pointer> _Head;   // 栈顶
    allocator_type _Allocator;          // 分配器

public:
    stack()
        : _Head(nullptr)
    {
    }

public:
    /**
     * @brief 入栈
     * @return 入栈成功
     */
    bool push(const value_type & _Value)
    {
        _Node_pointer _New_node = _Create_node(_Value);
        _New_node->_Next = _Head.load();
        while (!_Head.compare_exchange_weak(_New_node->_Next, _New_node));
        return true;
    }

    /**
     * @brief 出栈
     * @param _Value 出栈数据
     * @return true 出栈成功 | false 栈为空
     */
    bool pop(value_type & _Value)
    {
        // 第一时间申请本线程的风险指针
        thread_local hazard_pointer * _Hp = nullptr;

        // 搜索风险指针列表
        for (std::size_t i = 0; i < _Hazard_pointer_max; ++i) {
            std::thread::id _Old_id;        // 线程ID
            if (_Hazard_pointer_list[i]._Thread_id.compare_exchange_strong(_Old_id, std::this_thread::get_id())) {
                _Hp = &_Hazard_pointer_list[i];
                break;
            }
        }

        if (!_Hp) {
            // 无空闲风险指针
            throw std::runtime_error("Hazard pointer list exhausted");
        }
        
        /**
         * 保护栈顶节点
         * 保护的目的是：当该线程正在取出栈顶节点时，其他线程不能够将这个节点删除 
         */
        _Node_pointer _Old_head = _Head.load();
        do {
            _Hp->protect(_Head);
        } while (_Old_head && !_Head.compare_exchange_weak(_Old_head, _Old_head->_Next));   // 确保栈不为空且保护后状态未被改变

        /** 
         * 已经取出栈顶节点，可以取消保护了
         * 这是因为在保护节点时，已经将_head指向了原栈顶的下一个节点
         * 在取出该节点后，不会再有任何线程能够再访问到了
         */
        _Hp->reset_protection();

        // 取出值
        if (!_Old_head) {
            // 栈为空
            return false;
        }

        // 栈不为空
        _Value = _Old_head->_Value;
        
        /**
         * 准备删除节点，这里还要判断是否有其他线程持有
         * 假设以下情景：
         * 线程A、B同时pop，线程A顺利获取了head并移动了栈顶指针
         * 线程B也顺利获取了head，但是由于A移动了栈顶，compare失败了
         * 需要继续判断head->next，此时两个线程都持有了head资源
         * 如果不判断就删除，则线程B可能访问已释放的内存
         */
        for (std::size_t i = 0; i < _Hazard_pointer_max; ++i) {
            if (_Hazard_pointer_list[i]._Protect_ptr.load() == _Old_head) {
                // 其他线程持有该节点，标记为延迟删除
                _Old_head->_Should_delay = true;
                break;
            }
        }

        // 尝试删除节点
        _Destroy_node(_Old_head);

        /**
         * 尝试删除延迟删除的节点
         * 所有节点最终都会被删除，不会造成内存泄漏 
         */
        _Reclaim_list_instance.release();

        return true;
    }

public:
    /**
     * @brief 申请一个节点
     */
    _Node_pointer _Get_node()
    {
        return _Allocator.allocate(1);
    }

    /**
     * @brief 释放一个节点
     */
    void _Put_node(_Node_pointer _Node)
    {
        _Allocator.deallocate(_Node, 1);
    }

    /**
     * @brief 创建一个节点
     */
    template <typename... _ValTy>
    _Node_pointer _Create_node(_ValTy&&... _Vals)
    {
        _Node_pointer _New_node = _Get_node();
        _Allocator.construct(_New_node, std::forward<_ValTy>(_Vals)...);
        return _New_node;
    }

    /**
     * @brief 销毁一个节点
     */
    void _Destroy_node(_Node_pointer _Node)
    {
        _Allocator.destroy(_Node);
        _Put_node(_Node);
    }
};

} // namespace wwlockfree
