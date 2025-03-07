#pragma once

#include <atomic>

#include "ww_memory.h"

namespace wwlockfree
{

/**
 * @brief stack node
 * @tparam _Ty 数据类型
 */
template <typename _Ty>
class _Stack_node
{
public:
    using value_type = _Ty;
    using _Node_pointer = _Stack_node<_Ty>*;

public:
    value_type _Value;          // 数据
    _Node_pointer _Next;        // 后继指针

public:

    _Stack_node(const value_type & _New_value)
        : _Value(_New_value), _Next(nullptr)
    {
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
    using _Node_pointer = _Stack_node<_Ty>*;

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
        _Node_pointer _Old_head = _Head.load();
        while (_Old_head && !_Head.compare_exchange_weak(_Old_head, _Old_head->_Next));

        if (!_Old_head)
            return false;
        
        _Value = _Old_head->_Value;
        _Destroy_node(_Old_head);
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
