#pragma once

#include <cstddef>
#include <cstdint>
#include <cstring>
#include <initializer_list>
#include <iterator>
#include <limits>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

namespace Limen
{

    /**
     * 连续存储并独占元素生命周期的动态数组。
     * 扩容时仅对平凡可复制的 T 按字节搬迁；其他 T 逐元素复制或移动构造。
     * 长度和容量使用 uint32_t；这不是提供全部 std::vector 操作的兼容接口。
     * 发生重分配后，原有元素的指针、引用和迭代器失效。
     */
    template<typename T>
    class LArray
    {
    private:
        using Allocator = std::allocator<T>;
        using AllocatorTraits = std::allocator_traits<Allocator>;
    public:
        LArray() = default;

        explicit LArray(const uint32_t n)
        {
            T *newData = Allocate(n);
            uint32_t constructed = 0;
            try
            {
                for (; constructed < n; ++constructed)
                    AllocatorTraits::construct(allocator, newData + constructed);
            } catch (...)
            {
                DestroyElements(newData, constructed);
                Deallocate(newData, n);
                throw;
            }

            data = newData;
            size = n;
            capacity = n;
        }

        LArray(const uint32_t n, const T &value)
        {
            T *newData = Allocate(n);
            uint32_t constructed = 0;
            try
            {
                for (; constructed < n; ++constructed)
                    AllocatorTraits::construct(allocator, newData + constructed, value);
            } catch (...)
            {
                DestroyElements(newData, constructed);
                Deallocate(newData, n);
                throw;
            }

            data = newData;
            size = n;
            capacity = n;
        }

        LArray(std::initializer_list<T> list)
        {
            if (list.size() > std::numeric_limits<uint32_t>::max())
                throw std::length_error("LArray initializer list exceeds uint32_t range");

            const uint32_t count = static_cast<uint32_t>(list.size());
            T *newData = Allocate(count);
            uint32_t constructed = 0;
            try
            {
                for (const T &item: list)
                {
                    AllocatorTraits::construct(allocator, newData + constructed, item);
                    ++constructed;
                }
            } catch (...)
            {
                DestroyElements(newData, constructed);
                Deallocate(newData, count);
                throw;
            }

            data = newData;
            size = count;
            capacity = count;
        }

        LArray(const LArray &other)
        {
            T *newData = Allocate(other.capacity);
            uint32_t constructed = 0;
            try
            {
                for (; constructed < other.size; ++constructed)
                    AllocatorTraits::construct(allocator, newData + constructed, other.data[constructed]);
            } catch (...)
            {
                DestroyElements(newData, constructed);
                Deallocate(newData, other.capacity);
                throw;
            }

            data = newData;
            size = other.size;
            capacity = other.capacity;
        }

        LArray(LArray &&other) noexcept
            : data(std::exchange(other.data, nullptr)),
              size(std::exchange(other.size, 0)),
              capacity(std::exchange(other.capacity, 0))
        {
        }

        LArray &operator=(const LArray &other)
        {
            if (this != &other)
            {
                LArray copy(other);
                Swap(copy);
            }
            return *this;
        }

        LArray &operator=(LArray &&other) noexcept
        {
            if (this != &other)
            {
                ReleaseStorage();
                data = std::exchange(other.data, nullptr);
                size = std::exchange(other.size, 0);
                capacity = std::exchange(other.capacity, 0);
            }
            return *this;
        }

        ~LArray()
        {
            ReleaseStorage();
        }

        // 只预留存储，不改变 size，也不构造新元素。
        void reserve(const uint32_t newCapacity)
        {
            if (newCapacity > capacity)
                Reallocate(newCapacity);
        }

        void clear() noexcept
        {
            DestroyElements(data, size);
            size = 0;
        }

        // 空数组没有末尾元素；显式报错，避免无符号 size 下溢。
        void pop_back()
        {
            if (size == 0)
                throw std::out_of_range("LArray::pop_back on empty array");

            --size;
            AllocatorTraits::destroy(allocator, data + size);
        }

        void push_back(const T &value)
        {
            emplace_back(value);
        }

        void push_back(T &&value)
        {
            emplace_back(std::move(value));
        }

        template<typename... Args>
        void emplace_back(Args &&... args)
        {
            if (size < capacity)
            {
                AllocatorTraits::construct(allocator, data + size, std::forward<Args>(args)...);
                ++size;
                return;
            }

            const uint32_t newCapacity = NextCapacity();
            T *newData = Allocate(newCapacity);
            uint32_t relocated = 0;
            bool appended = false;
            try
            {
                // 先构造新元素，避免参数引用旧数组元素时在搬迁后悬空。
                AllocatorTraits::construct(allocator, newData + size, std::forward<Args>(args)...);
                appended = true;
                if constexpr (std::is_trivially_copyable_v<T>)
                {
                    // 只搬迁旧数组中已构造的元素；空数组不能向 memcpy 传入空指针。
                    if (size != 0)
                    {
                        std::memcpy(newData, data, sizeof(T) * size);
                        relocated = size;
                    }
                }
                else
                {
                    // 有资源所有权的类型仍须执行复制或移动构造。
                    for (; relocated < size; ++relocated)
                    {
                        AllocatorTraits::construct(
                            allocator, newData + relocated,
                            std::move_if_noexcept(data[relocated])
                        );
                    }
                }
            } catch (...)
            {
                DestroyElements(newData, relocated);
                if (appended)
                    AllocatorTraits::destroy(allocator, newData + size);
                Deallocate(newData, newCapacity);
                throw;
            }

            const uint32_t oldSize = size;
            ReleaseStorage();
            data = newData;
            size = oldSize + 1;
            capacity = newCapacity;
        }

        T &operator[](std::size_t index) noexcept { return data[index]; }
        const T &operator[](std::size_t index) const noexcept { return data[index]; }

        uint32_t getSize() const noexcept { return size; }
        uint32_t getCapacity() const noexcept { return capacity; }
        bool empty() const noexcept { return size == 0; }

        // 返回非拥有首元素指针；重分配或数组销毁后失效，clear/pop_back 后不能读取已销毁元素。
        [[nodiscard]] T *getData() noexcept { return data; }
        [[nodiscard]] const T *getData() const noexcept { return data; }

        T *begin() noexcept { return data; }
        const T *begin() const noexcept { return data; }
        const T *cbegin() const noexcept { return data; }

        T *end() noexcept { return size == 0 ? data : data + size; }
        const T *end() const noexcept { return size == 0 ? data : data + size; }
        const T *cend() const noexcept { return size == 0 ? data : data + size; }

        std::reverse_iterator<T *> rbegin() noexcept { return std::reverse_iterator<T *>(end()); }
        std::reverse_iterator<T *> rend() noexcept { return std::reverse_iterator<T *>(begin()); }
        std::reverse_iterator<const T *> rbegin() const noexcept { return std::reverse_iterator<const T *>(end()); }
        std::reverse_iterator<const T *> rend() const noexcept { return std::reverse_iterator<const T *>(begin()); }
        std::reverse_iterator<const T *> crbegin() const noexcept { return std::reverse_iterator<const T *>(cend()); }
        std::reverse_iterator<const T *> crend() const noexcept { return std::reverse_iterator<const T *>(cbegin()); }

    private:
        // capacity 个槽位只是预留存储；只有前 size 个元素真正构造。
        T *data = nullptr;
        uint32_t size = 0;
        uint32_t capacity = 0;
        Allocator allocator;

        T *Allocate(uint32_t count)
        {
            return count == 0 ? nullptr : AllocatorTraits::allocate(allocator, count);
        }

        void Deallocate(T *storage, uint32_t count) noexcept
        {
            if (storage != nullptr)
                AllocatorTraits::deallocate(allocator, storage, count);
        }

        void DestroyElements(T *storage, uint32_t count) noexcept
        {
            while (count > 0)
            {
                --count;
                AllocatorTraits::destroy(allocator, storage + count);
            }
        }

        void ReleaseStorage() noexcept
        {
            DestroyElements(data, size);
            Deallocate(data, capacity);
            data = nullptr;
            size = 0;
            capacity = 0;
        }

        uint32_t NextCapacity() const
        {
            constexpr uint32_t maxCapacity = std::numeric_limits<uint32_t>::max();
            if (size == maxCapacity)
                throw std::length_error("LArray capacity exceeds uint32_t range");

            if (capacity == 0)
                return 1;
            return capacity > maxCapacity / 2 ? maxCapacity : capacity * 2;
        }

        // 先准备好新存储，再释放旧存储；非平凡类型必须逐个构造以维护资源所有权。
        // 构造中途失败时，清理已构造的新元素，旧数组仍可使用。
        // 若 T 只能进行可能抛异常的移动，旧元素可能已被部分移出。
        void Reallocate(const uint32_t newCapacity)
        {
            T *newData = Allocate(newCapacity);
            uint32_t constructed = 0;
            try
            {
                if constexpr (std::is_trivially_copyable_v<T>)
                {
                    // 只复制实际存在的 size 个元素；未使用的 capacity 槽位尚未构造。
                    // 空数组不调用 memcpy，避免传入空指针。
                    if (size != 0)
                    {
                        std::memcpy(newData, data, sizeof(T) * size);
                        constructed = size;
                    }
                }
                else
                {
                    for (; constructed < size; ++constructed)
                    {
                        AllocatorTraits::construct(
                            allocator, newData + constructed,
                            std::move_if_noexcept(data[constructed])
                        );
                    }
                }
            }
            catch (...)
            {
                DestroyElements(newData, constructed);
                Deallocate(newData, newCapacity);
                throw;
            }

            const uint32_t oldSize = size;
            ReleaseStorage();
            data = newData;
            size = oldSize;
            capacity = newCapacity;
        }

        void Swap(LArray &other) noexcept
        {
            std::swap(data, other.data);
            std::swap(size, other.size);
            std::swap(capacity, other.capacity);
        }
    };
}
