//
// Created by chenlong on 2026/10/13.
//

#pragma once

#include <concepts>
#include <cstddef>
#include <cstdint>
#include <cmath>
#include <functional>
#include <iterator>
#include <limits>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "Limen/Tool/LArray.h"

namespace Limen
{
    /**
     * 自有节点式哈希映射。桶处理哈希冲突，独立节点拥有不可修改的 Key 和 Value。
     * rehash 只重连桶，不移动节点，因此元素引用和指针保持有效。
     * 迭代顺序是插入顺序；不要依赖它来代替有序映射。
     */
    template<typename K, typename V, typename Hash = std::hash<K>,
             typename KeyEqual = std::equal_to<K>>
    class LMap
    {
    public:
        using key_type = K;
        using mapped_type = V;
        // 保留 first/second，供已有调用点与结构化绑定使用；条目本身不依赖 std::pair。
        struct Entry
        {
            const K first;
            V second;

            template<typename KeyArg, typename... ValueArgs>
            explicit Entry(KeyArg &&key, ValueArgs &&...valueArgs)
                : first(std::forward<KeyArg>(key)),
                  second(std::forward<ValueArgs>(valueArgs)...)
            {
            }
        };

        using value_type = Entry;
        using size_type = std::size_t;
        using difference_type = std::ptrdiff_t;
        using hasher = Hash;
        using key_equal = KeyEqual;
        using reference = value_type &;
        using const_reference = const value_type &;
        using pointer = value_type *;
        using const_pointer = const value_type *;

    private:
        struct Node
        {
            Node *BucketNext = nullptr;
            Node *OrderPrev = nullptr;
            Node *OrderNext = nullptr;
            size_type CachedHash;
            value_type Entry;

            template<typename KeyArg, typename... ValueArgs>
            Node(const size_type hash, KeyArg &&key, ValueArgs &&...valueArgs)
                : CachedHash(hash),
                  Entry(std::forward<KeyArg>(key),
                        std::forward<ValueArgs>(valueArgs)...)
            {
            }
        };

    public:
        template<bool IsConst>
        class IteratorBase
        {
            friend class LMap;
            template<bool>
            friend class IteratorBase;

            using node_pointer = std::conditional_t<IsConst, const Node *, Node *>;

        public:
            using iterator_category = std::forward_iterator_tag;
            using value_type = value_type;
            using difference_type = difference_type;
            using reference = std::conditional_t<IsConst, const value_type &, value_type &>;
            using pointer = std::conditional_t<IsConst, const value_type *, value_type *>;

            constexpr IteratorBase() noexcept = default;
            constexpr IteratorBase(const IteratorBase &) noexcept = default;
            constexpr IteratorBase &operator=(const IteratorBase &) noexcept = default;

            explicit constexpr IteratorBase(const IteratorBase<false> &other) noexcept
                requires IsConst
                : m_Node(other.m_Node)
            {
            }

            constexpr reference operator*() const noexcept { return m_Node->Entry; }
            constexpr pointer operator->() const noexcept { return &m_Node->Entry; }

            constexpr IteratorBase &operator++() noexcept
            {
                m_Node = m_Node->OrderNext;
                return *this;
            }

            constexpr IteratorBase operator++(int) noexcept
            {
                IteratorBase previous = *this;
                ++*this;
                return previous;
            }

            template<bool OtherConst>
            constexpr bool operator==(const IteratorBase<OtherConst> &other) const noexcept
            {
                return m_Node == other.m_Node;
            }

            template<bool OtherConst>
            constexpr bool operator!=(const IteratorBase<OtherConst> &other) const noexcept
            {
                return m_Node != other.m_Node;
            }

        private:
            explicit constexpr IteratorBase(node_pointer node) noexcept : m_Node(node) { }
            node_pointer m_Node = nullptr;
        };

        using iterator = IteratorBase<false>;
        using const_iterator = IteratorBase<true>;

        // 与插入调用点约定：first 为元素位置，second 表示是否新插入。
        struct InsertResult
        {
            iterator first;
            bool second;
        };

        LMap() = default;

        explicit LMap(const size_type initialBucketCount, const Hash &hash = Hash{},
                      const KeyEqual &equal = KeyEqual{})
            : m_Hash(hash), m_Equal(equal)
        {
            rehash(initialBucketCount);
        }

        LMap(const LMap &other)
            requires (std::copy_constructible<K> && std::copy_constructible<V> &&
                      std::copy_constructible<Hash> && std::copy_constructible<KeyEqual>)
            : m_Hash(other.m_Hash), m_Equal(other.m_Equal),
              m_MaxLoadFactor(other.m_MaxLoadFactor)
        {
            // 构造函数抛异常时不会调用本对象析构函数，必须清理已经复制的节点。
            try
            {
                rehash(other.bucket_count());
                for (const auto &[key, value] : other)
                    try_emplace(key, value);
            }
            catch (...)
            {
                ReleaseStorage();
                throw;
            }
        }

        LMap(LMap &&other) noexcept(std::is_nothrow_move_constructible_v<Hash> &&
                                   std::is_nothrow_move_constructible_v<KeyEqual>)
            : m_Hash(std::move(other.m_Hash)),
              m_Equal(std::move(other.m_Equal)),
              m_Buckets(std::move(other.m_Buckets)),
              m_MaxLoadFactor(std::exchange(other.m_MaxLoadFactor, 1.0f)),
              m_Size(std::exchange(other.m_Size, 0)),
              m_Head(std::exchange(other.m_Head, nullptr)),
              m_Tail(std::exchange(other.m_Tail, nullptr))
        {
        }

        ~LMap() { ReleaseStorage(); }

        LMap &operator=(const LMap &other)
            requires (std::copy_constructible<K> && std::copy_constructible<V> &&
                      std::copy_constructible<Hash> && std::copy_constructible<KeyEqual>)
        {
            if (this != &other)
            {
                LMap copy(other);
                Swap(copy);
            }
            return *this;
        }

        LMap &operator=(LMap &&other)
            noexcept(std::is_nothrow_move_assignable_v<Hash> &&
                     std::is_nothrow_move_assignable_v<KeyEqual>)
        {
            if (this != &other)
            {
                ReleaseStorage();
                m_Hash = std::move(other.m_Hash);
                m_Equal = std::move(other.m_Equal);
                m_Buckets = std::move(other.m_Buckets);
                m_MaxLoadFactor = std::exchange(other.m_MaxLoadFactor, 1.0f);
                m_Size = std::exchange(other.m_Size, 0);
                m_Head = std::exchange(other.m_Head, nullptr);
                m_Tail = std::exchange(other.m_Tail, nullptr);
            }
            return *this;
        }

        [[nodiscard]] size_type size() const noexcept { return m_Size; }
        [[nodiscard]] bool empty() const noexcept { return m_Size == 0; }
        [[nodiscard]] size_type bucket_count() const noexcept { return m_Buckets.getSize(); }
        [[nodiscard]] float load_factor() const noexcept
        {
            return bucket_count() == 0
                       ? 0.0f
                       : static_cast<float>(m_Size) / static_cast<float>(bucket_count());
        }
        [[nodiscard]] float max_load_factor() const noexcept { return m_MaxLoadFactor; }

        // 负载因子上限属于容器实例；降低上限时先扩桶，失败则保持原规则。
        void max_load_factor(const float factor)
        {
            if (!std::isfinite(factor) || factor <= 0.0f)
                throw std::invalid_argument("LMap: max load factor must be finite and positive");
            if (const size_type required = RequiredBucketCount(m_Size, factor); required > bucket_count())
                rehash(required);
            m_MaxLoadFactor = factor;
        }

        iterator begin() noexcept { return iterator(m_Head); }
        const_iterator begin() const noexcept { return const_iterator(m_Head); }
        const_iterator cbegin() const noexcept { return const_iterator(m_Head); }
        iterator end() noexcept { return iterator(nullptr); }
        const_iterator end() const noexcept { return const_iterator(nullptr); }
        const_iterator cend() const noexcept { return const_iterator(nullptr); }

        iterator find(const key_type &key)
        {
            return iterator(FindNode(key, m_Hash(key)));
        }

        const_iterator find(const key_type &key) const
        {
            return const_iterator(FindNode(key, m_Hash(key)));
        }

        [[nodiscard]] bool contains(const key_type &key) const
        {
            return FindNode(key, m_Hash(key)) != nullptr;
        }

        [[nodiscard]] size_type count(const key_type &key) const
        {
            return contains(key) ? 1 : 0;
        }

        mapped_type &at(const key_type &key)
        {
            Node *node = FindNode(key, m_Hash(key));
            if (node == nullptr)
                throw std::out_of_range("LMap: key not found");
            return node->Entry.second;
        }

        const mapped_type &at(const key_type &key) const
        {
            const Node *node = FindNode(key, m_Hash(key));
            if (node == nullptr)
                throw std::out_of_range("LMap: key not found");
            return node->Entry.second;
        }

        mapped_type &operator[](const key_type &key)
        {
            return try_emplace(key).first->second;
        }

        mapped_type &operator[](key_type &&key)
        {
            return try_emplace(std::move(key)).first->second;
        }

        template<typename KeyArg, typename... ValueArgs>
        InsertResult try_emplace(KeyArg &&key, ValueArgs &&...valueArgs)
        {
            return TryEmplaceImpl(std::forward<KeyArg>(key),
                                  std::forward<ValueArgs>(valueArgs)...);
        }

        template<typename KeyArg, typename... ValueArgs>
        InsertResult emplace(KeyArg &&key, ValueArgs &&...valueArgs)
        {
            return TryEmplaceImpl(std::forward<KeyArg>(key),
                                  std::forward<ValueArgs>(valueArgs)...);
        }

        InsertResult insert(const value_type &value)
        {
            return try_emplace(value.first, value.second);
        }

        InsertResult insert(value_type &&value)
        {
            return try_emplace(value.first, std::move(value.second));
        }

        template<typename KeyArg, typename ValueArg>
        InsertResult insert_or_assign(KeyArg &&key, ValueArg &&value)
        {
            auto result = try_emplace(std::forward<KeyArg>(key),
                                      std::forward<ValueArg>(value));
            if (!result.second)
                result.first->second = std::forward<ValueArg>(value);
            return result;
        }

        size_type erase(const key_type &key)
        {
            Node *node = FindNode(key, m_Hash(key));
            if (node == nullptr)
                return 0;
            EraseNode(node);
            return 1;
        }

        iterator erase(const_iterator position)
        {
            if (position.m_Node == nullptr)
                return end();
            return EraseNode(const_cast<Node *>(position.m_Node));
        }

        void clear() noexcept
        {
            Node *node = m_Head;
            while (node != nullptr)
            {
                Node *next = node->OrderNext;
                delete node;
                node = next;
            }
            m_Head = nullptr;
            m_Tail = nullptr;
            m_Size = 0;
            for (size_type index = 0; index < bucket_count(); ++index)
                m_Buckets[index] = nullptr;
        }

        // reserve 以元素数为参数，依据当前最大负载因子换算为桶数。
        void reserve(const size_type elementCount)
        {
            if (const size_type required = RequiredBucketCount(elementCount, m_MaxLoadFactor);
                required > bucket_count()
                )
                rehash(required);
        }

        void rehash(const size_type requestedBucketCount)
        {
            const size_type minimum = RequiredBucketCount(m_Size, m_MaxLoadFactor);
            const size_type newCount =
                requestedBucketCount < minimum ? minimum : requestedBucketCount;
            if (newCount > std::numeric_limits<uint32_t>::max())
                throw std::length_error("LMap: bucket count exceeds LArray range");
            if (newCount == bucket_count())
                return;

            // 先让 LArray 分配并初始化桶；成功后才重连节点，节点地址始终不变。
            LArray<Node *> newBuckets(static_cast<uint32_t>(newCount), nullptr);

            for (Node *node = m_Head; node != nullptr; node = node->OrderNext)
            {
                const size_type bucket = node->CachedHash % newCount;
                node->BucketNext = newBuckets[bucket];
                newBuckets[bucket] = node;
            }

            m_Buckets = std::move(newBuckets);
        }

        void Swap(LMap &other)
            noexcept(std::is_nothrow_swappable_v<Hash> &&
                     std::is_nothrow_swappable_v<KeyEqual>)
        {
            using std::swap;
            swap(m_Hash, other.m_Hash);
            swap(m_Equal, other.m_Equal);
            swap(m_Buckets, other.m_Buckets);
            swap(m_MaxLoadFactor, other.m_MaxLoadFactor);
            swap(m_Size, other.m_Size);
            swap(m_Head, other.m_Head);
            swap(m_Tail, other.m_Tail);
        }

    private:
        [[nodiscard]] Node *FindNode(const key_type &key, size_type hash) const
        {
            if (bucket_count() == 0)
                return nullptr;
            for (Node *node = m_Buckets[hash % bucket_count()];
                 node != nullptr; node = node->BucketNext)
            {
                if (node->CachedHash == hash && m_Equal(node->Entry.first, key))
                    return node;
            }
            return nullptr;
        }

        template<typename KeyArg, typename... ValueArgs>
        InsertResult TryEmplaceImpl(KeyArg &&key, ValueArgs &&...valueArgs)
        {
            const size_type hash = m_Hash(key);
            if (Node *existing = FindNode(key, hash))
                return {.first = iterator(existing), .second = false};

            // 只有扩桶仍可能失败；失败时释放尚未接入容器的新节点。
            Node *node = new Node(hash, std::forward<KeyArg>(key),
                                  std::forward<ValueArgs>(valueArgs)...);
            try
            {
                EnsureCapacityForInsert();
            }
            catch (...)
            {
                delete node;
                throw;
            }

            const size_type bucket = hash % bucket_count();
            node->BucketNext = m_Buckets[bucket];
            m_Buckets[bucket] = node;
            node->OrderPrev = m_Tail;
            if (m_Tail != nullptr)
                m_Tail->OrderNext = node;
            else
                m_Head = node;
            m_Tail = node;
            ++m_Size;
            return {.first = iterator(node), .second = true};
        }

        void EnsureCapacityForInsert()
        {
            if (m_Size == std::numeric_limits<size_type>::max())
                throw std::length_error("LMap: size exceeds size_type range");
            const size_type required = RequiredBucketCount(m_Size + 1, m_MaxLoadFactor);
            if (required <= bucket_count())
                return;

            constexpr size_type initialBucketCount = 8;
            constexpr size_type maximum = std::numeric_limits<uint32_t>::max();
            size_type newCount = initialBucketCount;
            if (bucket_count() != 0)
                newCount = bucket_count() > maximum / 2
                               ? maximum
                               : bucket_count() * 2;
            if (newCount < required)
                newCount = required;
            rehash(newCount);
        }

        iterator EraseNode(Node *node)
        {
            if (bucket_count() == 0)
                throw std::invalid_argument("LMap: iterator does not belong to this map");
            Node **link = &m_Buckets[node->CachedHash % bucket_count()];
            while (*link != nullptr && *link != node)
                link = &(*link)->BucketNext;
            if (*link == nullptr)
                throw std::invalid_argument("LMap: iterator does not belong to this map");

            *link = node->BucketNext;
            Node *next = node->OrderNext;
            if (node->OrderPrev != nullptr)
                node->OrderPrev->OrderNext = node->OrderNext;
            else
                m_Head = node->OrderNext;
            if (node->OrderNext != nullptr)
                node->OrderNext->OrderPrev = node->OrderPrev;
            else
                m_Tail = node->OrderPrev;
            delete node;
            --m_Size;
            return iterator(next);
        }

        void ReleaseStorage() noexcept
        {
            clear();
            m_Buckets = LArray<Node *>{};
        }

        static size_type RequiredBucketCount(const size_type elementCount, const float factor)
        {
            if (elementCount == 0)
                return 0;
            const long double required = std::ceil(
                static_cast<long double>(elementCount) / static_cast<long double>(factor));
            if (required > static_cast<long double>(std::numeric_limits<uint32_t>::max()))
                throw std::length_error("LMap: bucket count exceeds LArray range");
            return static_cast<size_type>(required);
        }

        // 哈希器和相等比较器必须先于资源移动构造，避免其构造抛异常后资源失去所有者。
        [[no_unique_address]] Hash m_Hash{};
        [[no_unique_address]] KeyEqual m_Equal{};
        LArray<Node *> m_Buckets;
        float m_MaxLoadFactor = 1.0f;
        size_type m_Size = 0;
        Node *m_Head = nullptr;
        Node *m_Tail = nullptr;
    };
}
