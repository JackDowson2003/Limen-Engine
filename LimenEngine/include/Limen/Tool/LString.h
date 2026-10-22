#pragma once

#include <algorithm>
#include <cassert>
#include <cctype>
#include <cstddef>
#include <cstring>
#include <functional>
#include <sstream>
#include <stdexcept>
#include <memory>
#include <utility>
#include <string>
#include <string_view>

namespace Limen
{

    class LString {
    public:
        using Allocator       = std::allocator<char>;
        using AllocatorTraits = std::allocator_traits<Allocator>;

        using value_type      = char;
        using size_type       = std::size_t;
        using difference_type = std::ptrdiff_t;
        using reference       = char&;
        using const_reference = const char&;
        using pointer         = char*;
        using const_pointer   = const char*;
        using iterator        = char*;
        using const_iterator  = const char*;

        static constexpr size_type npos = static_cast<size_type>(-1);

    private:
        char*     data_;      // 字符数据
        size_type size_;      // 长度（不含 '\0'）
        size_type capacity_;  // 容量（不含 '\0'）
        Allocator alloc_;     // 分配器实例

        // 移动后的对象允许不持有堆缓冲区；只读访问仍返回有效的空串。
        static char* emptyData() noexcept {
            static char empty = '\0';
            return &empty;
        }

        [[nodiscard]] size_type maxCapacity() const noexcept {
            return AllocatorTraits::max_size(alloc_) - 1;
        }

        [[nodiscard]] bool isOwnData(const char* source) const noexcept {
            if (!data_ || !source) return false;
            constexpr std::less<const char*> less;
            return !less(source, data_) && !less(data_ + size_, source);
        }

        char* allocateRaw(size_type cap) {
            if (cap > maxCapacity())
                throw std::length_error("LString capacity exceeds allocator limit");
            return AllocatorTraits::allocate(alloc_, cap + 1); // +1 给 '\0'
        }

        void deallocateRaw(char* p, size_type cap) noexcept {
            if (p) AllocatorTraits::deallocate(alloc_, p, cap + 1);
        }

        void reallocate(size_type newCap) {
            char* newData = allocateRaw(newCap);
            if (data_) {
                if (size_) std::memcpy(newData, data_, size_);
                newData[size_] = '\0';
                deallocateRaw(data_, capacity_);
            } else {
                newData[0] = '\0';
            }
            data_     = newData;
            capacity_ = newCap;
        }

        void ensureCapacity(size_type minCap) {
            if (minCap > maxCapacity())
                throw std::length_error("LString capacity exceeds allocator limit");
            if (data_ && minCap <= capacity_) return;
            size_type cap = capacity_ ? capacity_ : 15;
            while (cap < minCap) {
                if (cap > maxCapacity() / 2) { cap = minCap; break; }
                cap *= 2;
            }
            if (cap > maxCapacity()) cap = minCap;
            reallocate(cap);
        }

        void assignImpl(const char* src, size_type len) {
            if (len && !src)
                throw std::invalid_argument("LString source is null");
            // 先保存内部源数据，扩容后旧指针才不会悬空。
            if (len && isOwnData(src)) {
                LString copy(src, len, alloc_);
                assignImpl(copy.data_, len);
                return;
            }
            ensureCapacity(len);
            if (len) std::memmove(data_, src, len);
            size_ = len;
            data_[size_] = '\0';
        }

    public:
        // ============ 构造 ============

        LString() : data_(nullptr), size_(0), capacity_(0), alloc_() {
            ensureCapacity(15);
        }

        explicit LString(const Allocator& alloc)
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            ensureCapacity(15);
        }

        // 从 std::string_view 构造
        explicit LString(std::string_view sv, const Allocator& alloc = Allocator())
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            size_ = sv.size();
            ensureCapacity(size_);
            if (size_) std::memcpy(data_, sv.data(), size_);
            data_[size_] = '\0';   // 自己补结尾，不依赖 sv 是否以 '\0' 结尾
        }

        LString(const char* str, const Allocator& alloc = Allocator())
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            if (!str) { ensureCapacity(15); return; }
            size_type len = std::strlen(str);
            size_ = len;
            ensureCapacity(len);
            std::memcpy(data_, str, len + 1);
        }

        LString(const char* str, size_type len, const Allocator& alloc = Allocator())
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            if (len && !str)
                throw std::invalid_argument("LString source is null");
            size_ = len;
            ensureCapacity(len);
            if (len) std::memcpy(data_, str, len);
            data_[len] = '\0';
        }

        // 与文件系统、解析库等标准库边界交互时保留内嵌 NUL 字节。
        LString(const std::string& str, const Allocator& alloc = Allocator())
            : LString(str.data(), str.size(), alloc) {}

        LString(size_type count, char ch, const Allocator& alloc = Allocator())
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            size_ = count;
            ensureCapacity(count);
            if (count) std::memset(data_, ch, count);
            data_[count] = '\0';
        }

        // 拷贝构造
        LString(const LString& other)
            : data_(nullptr), size_(0), capacity_(0),
              alloc_(AllocatorTraits::select_on_container_copy_construction(other.alloc_)) {
            size_ = other.size_;
            ensureCapacity(size_);
            if (size_) std::memcpy(data_, other.data_, size_);
            data_[size_] = '\0';
        }

        LString(const LString& other, const Allocator& alloc)
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            size_ = other.size_;
            ensureCapacity(size_);
            if (size_) std::memcpy(data_, other.data_, size_);
            data_[size_] = '\0';
        }

        // 移动构造
        LString(LString&& other) noexcept
            : data_(other.data_), size_(other.size_),
              capacity_(other.capacity_), alloc_(std::move(other.alloc_)) {
            other.data_ = nullptr;
            other.size_ = other.capacity_ = 0;
        }

        LString(LString&& other, const Allocator& alloc)
            : data_(nullptr), size_(0), capacity_(0), alloc_(alloc) {
            if (alloc_ == other.alloc_) {
                data_     = other.data_;
                size_     = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = other.capacity_ = 0;
            } else {
                size_ = other.size_;
                ensureCapacity(size_);
                if (size_) std::memcpy(data_, other.data_, size_);
                data_[size_] = '\0';
                other.clear();
            }
        }

        ~LString() {
            deallocateRaw(data_, capacity_);
        }

        // ============ 赋值 ============

        LString& operator=(const LString& other) {
            if (this == &other) return *this;
            constexpr bool pocca =
                AllocatorTraits::propagate_on_container_copy_assignment::value;
            if (pocca && alloc_ != other.alloc_) {
                deallocateRaw(data_, capacity_);
                data_ = nullptr;
                size_ = capacity_ = 0;
            }
            alloc_ = other.alloc_;
            assignImpl(other.data_, other.size_);
            return *this;
        }

        LString& operator=(LString&& other) noexcept(
            AllocatorTraits::propagate_on_container_move_assignment::value ||
            AllocatorTraits::is_always_equal::value) {
            if (this == &other) return *this;
            constexpr bool pocma =
                AllocatorTraits::propagate_on_container_move_assignment::value;

            if (pocma) {
                deallocateRaw(data_, capacity_);
                data_     = other.data_;
                size_     = other.size_;
                capacity_ = other.capacity_;
                alloc_    = std::move(other.alloc_);
                other.data_ = nullptr;
                other.size_ = other.capacity_ = 0;
            } else if (alloc_ == other.alloc_) {
                deallocateRaw(data_, capacity_);
                data_     = other.data_;
                size_     = other.size_;
                capacity_ = other.capacity_;
                other.data_ = nullptr;
                other.size_ = other.capacity_ = 0;
            } else {
                assignImpl(other.data_, other.size_);
                other.clear();
            }
            return *this;
        }

        LString& operator=(const char* str) {
            assignImpl(str, str ? std::strlen(str) : 0);
            return *this;
        }

        LString& operator=(char ch) {
            assignImpl(&ch, 1);
            return *this;
        }

        Allocator get_allocator() const noexcept { return alloc_; }

        // ============ 容量 ============

        size_type size() const noexcept { return size_; }
        size_type length() const noexcept { return size_; }
        size_type capacity() const noexcept { return capacity_; }
        bool empty() const noexcept { return size_ == 0; }

        void reserve(size_type newCap) {
            if (newCap > capacity_) reallocate(newCap);
        }

        void resize(size_type newSize, char ch = '\0') {
            ensureCapacity(newSize);
            if (newSize > size_) {
                if (newSize > size_) std::memset(data_ + size_, ch, newSize - size_);
                data_[newSize] = '\0';
            } else {
                data_[newSize] = '\0';
            }
            size_ = newSize;
        }

        void clear() noexcept {
            size_ = 0;
            if (data_) data_[0] = '\0';
        }

        void shrink_to_fit() {
            const size_type target = std::max<size_type>(size_, 15);
            if (capacity_ > target) reallocate(target);
        }

        // ============ 元素访问 ============

        char& operator[](size_type i) { return begin()[i]; }
        const char& operator[](size_type i) const { return data()[i]; }

        char& at(size_type i) {
            if (i >= size_) throw std::out_of_range("LString::at");
            return data_[i];
        }
        const char& at(size_type i) const {
            if (i >= size_) throw std::out_of_range("LString::at");
            return data_[i];
        }

        char& front() { return data_[0]; }
        const char& front() const { return data_[0]; }
        char& back() { return data_[size_ - 1]; }
        const char& back() const { return data_[size_ - 1]; }

        const char* c_str() const noexcept { return data_ ? data_ : emptyData(); }
        const char* data() const noexcept { return c_str(); }

        // ============ 修改 ============

        LString& append(const char* str, size_type len) {
            if (len && !str)
                throw std::invalid_argument("LString source is null");
            if (len > maxCapacity() - size_)
                throw std::length_error("LString append exceeds allocator limit");
            // 自身子串也可能作为源；扩容会使原指针失效。
            if (len && isOwnData(str)) {
                LString copy(str, len, alloc_);
                return append(copy.data_, len);
            }
            ensureCapacity(size_ + len);
            if (len) std::memmove(data_ + size_, str, len);
            size_ += len;
            data_[size_] = '\0';
            return *this;
        }

        LString& append(const LString& other) {
            return append(other.data_, other.size_);
        }

        LString& append(const char* str) {
            return append(str, str ? std::strlen(str) : 0);
        }

        LString& append(size_type n, char ch) {
            if (n > maxCapacity() - size_)
                throw std::length_error("LString append exceeds allocator limit");
            ensureCapacity(size_ + n);
            if (n) std::memset(data_ + size_, ch, n);
            size_ += n;
            data_[size_] = '\0';
            return *this;
        }

        LString& operator+=(const LString& o) { return append(o); }
        LString& operator+=(const char* s)    { return append(s); }
        LString& operator+=(char c)           { return append(&c, 1); }

        void push_back(char c) { append(&c, 1); }

        void pop_back() {
            assert(size_ > 0);
            --size_;
            data_[size_] = '\0';
        }

        LString& insert(size_type pos, const char* str, size_type len) {
            if (pos > size_) throw std::out_of_range("LString::insert");
            if (len && !str)
                throw std::invalid_argument("LString source is null");
            if (len > maxCapacity() - size_)
                throw std::length_error("LString insert exceeds allocator limit");
            // 即使不扩容，移位也会覆盖来自本缓冲区的插入源。
            if (len && isOwnData(str)) {
                LString copy(str, len, alloc_);
                return insert(pos, copy.data_, len);
            }
            ensureCapacity(size_ + len);
            std::memmove(data_ + pos + len, data_ + pos, size_ - pos);
            if (len) std::memmove(data_ + pos, str, len);
            size_ += len;
            data_[size_] = '\0';
            return *this;
        }

        LString& insert(size_type pos, const LString& o) {
            return insert(pos, o.data_, o.size_);
        }

        LString& insert(size_type pos, const char* str) {
            return insert(pos, str, str ? std::strlen(str) : 0);
        }

        LString& erase(size_type pos = 0, size_type len = npos) {
            if (pos > size_) throw std::out_of_range("LString::erase");
            ensureCapacity(size_);
            if (len > size_ - pos) len = size_ - pos;
            std::memmove(data_ + pos, data_ + pos + len, size_ - pos - len);
            size_ -= len;
            data_[size_] = '\0';
            return *this;
        }

        // ============ 查找 ============

        size_type find(const char* str, size_type pos = 0) const {
            if (!str || pos > size_) return npos;
            size_type len = std::strlen(str);
            if (len == 0) return pos;
            if (len > size_ - pos) return npos;
            for (size_type i = pos; i + len <= size_; ++i) {
                if (std::memcmp(data_ + i, str, len) == 0) return i;
            }
            return npos;
        }

        size_type find(char ch, size_type pos = 0) const {
            for (size_type i = pos; i < size_; ++i) {
                if (data_[i] == ch) return i;
            }
            return npos;
        }

        LString substr(size_type pos = 0, size_type len = npos) const {
            if (pos > size_) throw std::out_of_range("LString::substr");
            if (len > size_ - pos) len = size_ - pos;
            return LString(data() + pos, len, alloc_);
        }

        int compare(const LString& o) const noexcept {
            size_type n = std::min(size_, o.size_);
            int r = n ? std::memcmp(data_, o.data_, n) : 0;
            if (r != 0) return r;
            if (size_ < o.size_) return -1;
            if (size_ > o.size_) return 1;
            return 0;
        }

        // 隐式转换到 std::string_view
        operator std::string_view() const noexcept {
            return std::string_view(data(), size_);
        }
        // ============ 迭代器 ============

        iterator begin() noexcept { return data_ ? data_ : emptyData(); }
        iterator end() noexcept { return begin() + size_; }
        const_iterator begin() const noexcept { return data(); }
        const_iterator end() const noexcept { return data() + size_; }
        const_iterator cbegin() const noexcept { return data(); }
        const_iterator cend() const noexcept { return data() + size_; }

        // ============ 友元运算符 ============

        friend LString operator+(const LString& lhs, const LString& rhs) {
            LString r(lhs.alloc_);
            if (rhs.size_ > r.maxCapacity() - lhs.size_)
                throw std::length_error("LString concatenation exceeds allocator limit");
            r.reserve(lhs.size_ + rhs.size_);
            r.append(lhs).append(rhs);
            return r;
        }

        friend LString operator+(const LString& lhs, const char* rhs) {
            return lhs + LString(rhs ? rhs : "", lhs.alloc_);
        }

        friend LString operator+(const char* lhs, const LString& rhs) {
            return LString(lhs ? lhs : "", rhs.alloc_) + rhs;
        }

        friend bool operator==(const LString& a, const LString& b) {
            return a.size_ == b.size_ &&
                   (a.size_ == 0 || std::memcmp(a.data_, b.data_, a.size_) == 0);
        }

        friend bool operator==(const LString& a, const char* b) {
            if (!b) return a.size_ == 0;
            size_type blen = std::strlen(b);
            return a.size_ == blen &&
                   (blen == 0 || std::memcmp(a.data_, b, blen) == 0);
        }

        friend bool operator!=(const LString& a, const LString& b) { return !(a == b); }
        friend bool operator!=(const LString& a, const char* b)     { return !(a == b); }

        friend bool operator<(const LString& a, const LString& b) { return a.compare(b) < 0; }

        friend std::ostream& operator<<(std::ostream& os, const LString& s) {
            os.write(s.data(), static_cast<std::streamsize>(s.size_));
            return os;
        }

        friend std::istream& operator>>(std::istream& is, LString& s) {
            std::istream::sentry sentry(is);
            if (!sentry) return is;
            s.clear();
            const std::streamsize width = is.width();
            const size_type limit = width > 0
                ? static_cast<size_type>(width)
                : s.maxCapacity();
            size_type read = 0;
            auto* streamBuffer = is.rdbuf();

            // 留下分隔空白供下一次提取；sentry 负责遵守 skipws/noskipws。
            while (read < limit) {
                const auto next = streamBuffer->sgetc();
                if (std::char_traits<char>::eq_int_type(next, std::char_traits<char>::eof())) {
                    is.setstate(std::ios::eofbit);
                    break;
                }
                const char ch = std::char_traits<char>::to_char_type(next);
                if (std::isspace(static_cast<unsigned char>(ch))) break;
                s.push_back(ch);
                streamBuffer->sbumpc();
                ++read;
            }
            is.width(0);
            if (read == 0) is.setstate(std::ios::failbit);
            return is;
        }
    };
}

template<>
struct std::hash<Limen::LString>
{
    size_t operator()(const Limen::LString& value) const noexcept
    {
        return hash<string_view>{}(string_view(value.data(), value.size()));
    }
};
