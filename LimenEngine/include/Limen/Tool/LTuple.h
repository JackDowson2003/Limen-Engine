//
// Created by chenlong on 2026/10/13.
//
#pragma once

#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

namespace Limen
{
    namespace TupleDetail
    {
        template<std::size_t... Indexes>
        struct LTupleIndexSequence {};

        template<std::size_t Count, std::size_t... Indexes>
        struct MakeLTupleIndexSequence
            : MakeLTupleIndexSequence<Count - 1, Count - 1, Indexes...> {};

        template<std::size_t... Indexes>
        struct MakeLTupleIndexSequence<0, Indexes...>
        {
            using type = LTupleIndexSequence<Indexes...>;
        };

        template<std::size_t Index, typename First, typename... Rest>
        struct LTupleTypeAt : LTupleTypeAt<Index - 1, Rest...> {};

        template<typename First, typename... Rest>
        struct LTupleTypeAt<0, First, Rest...>
        {
            using type = First;
        };

        template<std::size_t Index, typename T>
        struct LTupleLeaf
        {
            [[no_unique_address]] T Value;

            constexpr LTupleLeaf() requires std::is_default_constructible_v<T>
                : Value() {}

            template<typename Arg>
                requires std::is_constructible_v<T, Arg &&>
            constexpr explicit LTupleLeaf(Arg &&value)
                : Value(std::forward<Arg>(value)) {}
        };

        template<typename Indexes, typename... Types>
        struct LTupleStorage;

        template<std::size_t... Indexes, typename... Types>
        struct LTupleStorage<LTupleIndexSequence<Indexes...>, Types...>
            : LTupleLeaf<Indexes, Types>...
        {
            constexpr LTupleStorage()
                requires (std::is_default_constructible_v<Types> && ...) = default;

            template<typename... Args>
                requires (sizeof...(Args) == sizeof...(Types) &&
                          (std::is_constructible_v<Types, Args &&> && ...))
            constexpr explicit LTupleStorage(Args &&...args)
                : LTupleLeaf<Indexes, Types>(std::forward<Args>(args))... {}
        };
    }

    /**
     * 按值持有任意数量、不同类型的元素；LTuple<T&> 则只借用外部对象。
     * 各元素使用不同下标的基类存储，重复类型也能通过下标准确访问。
     * 不提供 std::tuple 的完整接口，只实现引擎目前需要的构造、Get 和结构化绑定。
     */
    template<typename... Types>
    class LTuple
        : private TupleDetail::LTupleStorage<
              typename TupleDetail::MakeLTupleIndexSequence<sizeof...(Types)>::type,
              Types...>
    {
        using Storage = TupleDetail::LTupleStorage<
            typename TupleDetail::MakeLTupleIndexSequence<sizeof...(Types)>::type,
            Types...>;

    public:
        constexpr LTuple() requires (std::is_default_constructible_v<Types> && ...) = default;
        constexpr LTuple(const LTuple &) = default;
        constexpr LTuple(LTuple &&) = default;
        constexpr LTuple &operator=(const LTuple &) = default;
        constexpr LTuple &operator=(LTuple &&) = default;

        template<typename... Args>
            requires (sizeof...(Types) > 0 && sizeof...(Args) == sizeof...(Types) &&
                      (std::is_constructible_v<Types, Args &&> && ...))
        constexpr explicit(!(std::is_convertible_v<Args &&, Types> && ...))
        LTuple(Args &&...args)
            : Storage(std::forward<Args>(args)...) {}

        template<std::size_t Index>
        [[nodiscard]] constexpr decltype(auto) Get() & noexcept
        {
            static_assert(Index < sizeof...(Types), "LTuple index out of range");
            using Element = typename TupleDetail::LTupleTypeAt<Index, Types...>::type;
            using Leaf = TupleDetail::LTupleLeaf<Index, Element>;
            return (static_cast<Leaf &>(*this).Value);
        }

        template<std::size_t Index>
        [[nodiscard]] constexpr decltype(auto) Get() const & noexcept
        {
            static_assert(Index < sizeof...(Types), "LTuple index out of range");
            using Element = typename TupleDetail::LTupleTypeAt<Index, Types...>::type;
            using Leaf = TupleDetail::LTupleLeaf<Index, Element>;
            return (static_cast<const Leaf &>(*this).Value);
        }

        template<std::size_t Index>
        [[nodiscard]] constexpr decltype(auto) Get() && noexcept
        {
            static_assert(Index < sizeof...(Types), "LTuple index out of range");
            using Element = typename TupleDetail::LTupleTypeAt<Index, Types...>::type;
            using Leaf = TupleDetail::LTupleLeaf<Index, Element>;
            return static_cast<Element &&>(static_cast<Leaf &>(*this).Value);
        }

        template<std::size_t Index>
        [[nodiscard]] constexpr decltype(auto) Get() const && noexcept
        {
            static_assert(Index < sizeof...(Types), "LTuple index out of range");
            using Element = typename TupleDetail::LTupleTypeAt<Index, Types...>::type;
            using Leaf = TupleDetail::LTupleLeaf<Index, Element>;
            return static_cast<const Element &&>(static_cast<const Leaf &>(*this).Value);
        }
    };

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) Get(LTuple<Types...> &value) noexcept
    {
        return value.template Get<Index>();
    }

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) Get(const LTuple<Types...> &value) noexcept
    {
        return value.template Get<Index>();
    }

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) Get(LTuple<Types...> &&value) noexcept
    {
        return std::move(value).template Get<Index>();
    }

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) Get(const LTuple<Types...> &&value) noexcept
    {
        return std::move(value).template Get<Index>();
    }

    // 结构化绑定的 tuple-like 协议会通过 ADL 查找小写 get。
    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) get(LTuple<Types...> &value) noexcept
    {
        return Get<Index>(value);
    }

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) get(const LTuple<Types...> &value) noexcept
    {
        return Get<Index>(value);
    }

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) get(LTuple<Types...> &&value) noexcept
    {
        return Get<Index>(std::move(value));
    }

    template<std::size_t Index, typename... Types>
    [[nodiscard]] constexpr decltype(auto) get(const LTuple<Types...> &&value) noexcept
    {
        return Get<Index>(std::move(value));
    }

    template<typename... Args>
    [[nodiscard]] constexpr auto MakeLTuple(Args &&...args)
    {
        return LTuple<std::decay_t<Args>...>(std::forward<Args>(args)...);
    }

    template<typename... Args>
    [[nodiscard]] constexpr auto TieLTuple(Args &...args) noexcept
    {
        return LTuple<Args &...>(args...);
    }
}

namespace std
{
    // 仅向标准 tuple-like 协议注册自有类型；实际元素仍由 Limen::LTuple 存储。
    template<typename... Types>
    struct tuple_size<Limen::LTuple<Types...>>
        : integral_constant<size_t, sizeof...(Types)> {};

    template<size_t Index, typename... Types>
    struct tuple_element<Index, Limen::LTuple<Types...>>
    {
        static_assert(Index < sizeof...(Types), "LTuple index out of range");
        using type = typename Limen::TupleDetail::LTupleTypeAt<Index, Types...>::type;
    };
}
