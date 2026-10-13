//
// Created by chenlong on 2026/10/13.
//

#pragma once

#include <algorithm>
#include <cstddef>
#include <concepts>
#include <initializer_list>
#include <stdexcept>
#include <utility>

namespace Limen
{
    /**
     * 编译期长度固定的数组；元素直接存放在 LStaticArray 对象内部。
     * N 是模板参数，不会作为运行时字段占用对象空间。
     */
    template<typename T, std::size_t N>
    class LStaticArray
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using reference = T&;
        using const_reference = const T&;
        using pointer = T*;
        using const_pointer = const T*;

        using iterator = T*;
        using const_iterator = const T*;

        // array{} 会值初始化元素；普通默认初始化与原生数组保持一致。
        constexpr LStaticArray() = default;

        // 花括号列表在调用处转换为 T，保留 unsigned char 等类型的窄化检查。
        // 不足 N 个值时值初始化剩余元素；超过 N 个值则抛出异常。
        constexpr LStaticArray(std::initializer_list<T> values)
            requires std::copy_constructible<T>
            : LStaticArray(values, std::make_index_sequence<N>{})
        {
        }

        // initializer_list 的元素是 const，不能从中移动；不可复制的 T 逐个构造。
        template<typename... Args>
            requires (!std::copy_constructible<T> && sizeof...(Args) == N &&
                      (std::constructible_from<T, Args &&> && ...))
        constexpr LStaticArray(Args &&...args)
            : m_Elements{std::forward<Args>(args)...}
        {
        }

        static constexpr size_type size() noexcept
        {
            return N;
        }

        constexpr pointer data() noexcept
        {
            return m_Elements;
        }

        constexpr const_pointer data() const noexcept
        {
            return m_Elements;
        }

        constexpr reference operator[](const size_type n) noexcept
        {
            return m_Elements[n];
        }

        constexpr const_reference operator[](const size_type n) const noexcept
        {
            return m_Elements[n];
        }

        constexpr void fill(const_reference val)
        {
            std::fill_n(data(), N, val);
        }

        constexpr reference at(const size_type n)
        {
            if (n >= N)
            {
                throw std::out_of_range("LStaticArray: index out of range");
            }
            return m_Elements[n];
        }

        constexpr const_reference at(const size_type n) const
        {
            if (n >= N)
            {
                throw std::out_of_range("LStaticArray: index out of range");
            }
            return m_Elements[n];
        }

        constexpr reference front()
        {
            return (*this)[0];
        }

        constexpr const_reference front() const
        {
            return (*this)[0];
        }

        constexpr reference back()
        {
            return m_Elements[N - 1];
        }

        constexpr const_reference back() const
        {
            return m_Elements[N - 1];
        }

        static constexpr bool empty() noexcept
        {
            return false;
        }

        static constexpr size_type max_size() noexcept
        {
            return N;
        }

        constexpr iterator begin() noexcept { return m_Elements; }
        constexpr const_iterator begin() const noexcept { return m_Elements; }
        constexpr const_iterator cbegin() const noexcept { return m_Elements; }

        constexpr iterator end() noexcept { return m_Elements + N; }
        constexpr const_iterator end() const noexcept { return m_Elements + N; }
        constexpr const_iterator cend() const noexcept { return m_Elements + N; }

        constexpr void Swap(LStaticArray &other)
        {
            std::swap(m_Elements, other.m_Elements);
        }

    private:
        static constexpr T ElementFromInitializer(std::initializer_list<T> values, size_type index)
        {
            if (index < values.size())
                return values.begin()[index];

            if constexpr (std::default_initializable<T>)
                return T{};
            else
                throw std::length_error("LStaticArray: missing initializer for non-default-constructible element");
        }

        template<std::size_t... Indices>
        constexpr LStaticArray(std::initializer_list<T> values, std::index_sequence<Indices...>)
            : m_Elements{ElementFromInitializer(values, Indices)...}
        {
            if (values.size() > N)
                throw std::length_error("LStaticArray: too many initializers");
        }

        T m_Elements[N];
    };

    /** 零长度数组不声明 T[0]，因此不需要创建任何 T 对象。 */
    template<typename T>
    class LStaticArray<T, 0>
    {
    public:
        using value_type = T;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;

        using reference = T &;
        using const_reference = const T &;
        using pointer = T *;
        using const_pointer = const T *;

        using iterator = T *;
        using const_iterator = const T *;

        static constexpr size_type size() noexcept { return 0; }
        static constexpr bool empty() noexcept { return true; }
        static constexpr size_type max_size() noexcept { return 0; }

        constexpr pointer data() noexcept { return nullptr; }
        constexpr const_pointer data() const noexcept { return nullptr; }

        // 没有可借用的元素：访问函数始终失败，不返回悬空引用。
        constexpr reference operator[](size_type) { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr const_reference operator[](size_type) const { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr reference at(size_type) { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr const_reference at(size_type) const { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr reference front() { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr const_reference front() const { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr reference back() { throw std::out_of_range("LStaticArray: empty array"); }
        constexpr const_reference back() const { throw std::out_of_range("LStaticArray: empty array"); }

        constexpr void fill(const_reference) noexcept { }

        // 空区间仅保证 begin() == end()；不要对空指针迭代器做减法。
        constexpr iterator begin() noexcept { return nullptr; }
        constexpr const_iterator begin() const noexcept { return nullptr; }
        constexpr const_iterator cbegin() const noexcept { return nullptr; }

        constexpr iterator end() noexcept { return nullptr; }
        constexpr const_iterator end() const noexcept { return nullptr; }
        constexpr const_iterator cend() const noexcept { return nullptr; }

        constexpr void Swap(LStaticArray &) { }
    };
}
