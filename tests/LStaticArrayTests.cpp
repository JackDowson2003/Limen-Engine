#include <cstddef>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include "Limen/Tool/LStaticArray.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    struct LifetimeProbe
    {
        static inline int alive = 0;

        LifetimeProbe() { ++alive; }
        ~LifetimeProbe() { --alive; }
    };

    struct Point
    {
        float x = 0.0f;
        float y = 0.0f;
    };

    struct NoDefault
    {
        static inline int alive = 0;

        NoDefault() = delete;
        explicit NoDefault(int) { ++alive; }
        ~NoDefault() { --alive; }
    };

    struct ThrowingSwap
    {
        int value = 0;

        friend void swap(ThrowingSwap &, ThrowingSwap &)
        {
            throw std::runtime_error("element swap failed");
        }
    };

    struct NonDefaultCopy
    {
        int value;

        NonDefaultCopy() = delete;
        explicit NonDefaultCopy(int newValue) : value(newValue) { }
    };

    using IntArray = Limen::LStaticArray<int, 3>;
    static_assert(sizeof(IntArray) == 3 * sizeof(int));
    static_assert(std::is_same_v<IntArray::value_type, int>);
    static_assert(std::is_same_v<IntArray::size_type, std::size_t>);
    static_assert(std::is_same_v<IntArray::difference_type, std::ptrdiff_t>);
    static_assert(std::is_same_v<IntArray::reference, int &>);
    static_assert(std::is_same_v<IntArray::const_reference, const int &>);
    static_assert(std::is_same_v<IntArray::pointer, int *>);
    static_assert(std::is_same_v<IntArray::const_pointer, const int *>);
    static_assert(std::is_same_v<IntArray::iterator, int *>);
    static_assert(std::is_same_v<IntArray::const_iterator, const int *>);

    constexpr IntArray kZero{};
    static_assert(kZero.size() == 3);
    static_assert(std::is_same_v<decltype(std::declval<IntArray &>().data()), int *>);
    static_assert(std::is_same_v<decltype(std::declval<const IntArray &>().data()), const int *>);
    static_assert(kZero.data() == &kZero[0]);
    static_assert(kZero.data()[1] == 0);

    // Renderer2D 的后备像素必须保持可在编译期直接花括号初始化。
    constexpr Limen::LStaticArray<unsigned char, 4> kFallbackPixel{255, 0, 255, 255};
    static_assert(kFallbackPixel.size() == 4);
    static_assert(kFallbackPixel[0] == 255 && kFallbackPixel[1] == 0 &&
                  kFallbackPixel[2] == 255 && kFallbackPixel[3] == 255);
    static_assert(kFallbackPixel.data()[2] == 255);

    constexpr Limen::LStaticArray<int, 3> kPartiallyInitialized{4, 5};
    static_assert(kPartiallyInitialized[0] == 4 && kPartiallyInitialized[1] == 5 &&
                  kPartiallyInitialized[2] == 0);

    using EmptyNoDefaultArray = Limen::LStaticArray<NoDefault, 0>;
    static_assert(std::is_default_constructible_v<EmptyNoDefaultArray>);
    static_assert(!noexcept(std::declval<Limen::LStaticArray<ThrowingSwap, 1> &>().Swap(
        std::declval<Limen::LStaticArray<ThrowingSwap, 1> &>())));
}

int main()
{
    bool passed = true;

    Limen::LStaticArray<int, 3> zero{};
    passed &= Check("value initialization initializes every element",
                    zero.size() == 3 && zero[0] == 0 &&
                    zero[1] == 0 && zero[2] == 0);

    int *mutableData = zero.data();
    passed &= Check("mutable data points to the first contiguous element",
                    mutableData == &zero[0] && mutableData + 2 == &zero[2]);
    mutableData[1] = 12;
    passed &= Check("writing through data updates the owned element",
                    zero[1] == 12);

    const auto &readOnly = zero;
    const int *constData = readOnly.data();
    passed &= Check("const data borrows the same elements",
                    constData == &readOnly[0] && constData[1] == 12);

    Limen::LStaticArray<int, 3> copy = zero;
    copy[0] = 10;
    passed &= Check("copy owns independent elements",
                    copy.size() == 3 && copy[0] == 10 && zero[0] == 0 &&
                    copy[1] == zero[1]);

    Limen::LStaticArray<std::unique_ptr<int>, 2> owned{};
    owned[0] = std::make_unique<int>(7);
    owned[1] = std::make_unique<int>(9);
    Limen::LStaticArray<std::unique_ptr<int>, 2> moved(std::move(owned));
    passed &= Check("move constructs move-only elements",
                    moved.size() == 2 && moved[0] && moved[1] &&
                    *moved[0] == 7 && *moved[1] == 9 &&
                    !owned[0] && !owned[1]);

    Limen::LStaticArray<std::unique_ptr<int>, 2> directlyOwned{
        std::make_unique<int>(11), std::make_unique<int>(13)
    };
    passed &= Check("brace construction moves non-copyable elements",
                    directlyOwned[0] && directlyOwned[1] &&
                    *directlyOwned[0] == 11 && *directlyOwned[1] == 13);

    Limen::LStaticArray<NonDefaultCopy, 2> nonDefault{
        NonDefaultCopy{31}, NonDefaultCopy{37}
    };
    passed &= Check("brace construction needs no default constructor for a full list",
                    nonDefault[0].value == 31 && nonDefault[1].value == 37);

    bool tooManyInitializersThrows = false;
    try { (void)Limen::LStaticArray<int, 2>{1, 2, 3}; }
    catch (const std::length_error &) { tooManyInitializersThrows = true; }
    passed &= Check("too many initializers are rejected", tooManyInitializersThrows);

    bool missingInitializerThrows = false;
    try { (void)Limen::LStaticArray<NonDefaultCopy, 2>{NonDefaultCopy{41}}; }
    catch (const std::length_error &) { missingInitializerThrows = true; }
    passed &= Check("missing non-default-constructible element is rejected", missingInitializerThrows);

    {
        Limen::LStaticArray<LifetimeProbe, 3> probes{};
        passed &= Check("all fixed elements are constructed",
                        probes.size() == 3 && LifetimeProbe::alive == 3);
    }
    passed &= Check("all fixed elements are destroyed with their array",
                    LifetimeProbe::alive == 0);

    Limen::LStaticArray<Point, 4> positions{};
    positions = {Point{-0.5f, -0.5f}, Point{0.5f, -0.5f},
                 Point{0.5f, 0.5f}, Point{-0.5f, 0.5f}};
    passed &= Check("brace assignment replaces every element",
                    positions[0].x == -0.5f && positions[0].y == -0.5f &&
                    positions[2].x == 0.5f && positions[2].y == 0.5f &&
                    positions[3].x == -0.5f && positions[3].y == 0.5f);

    zero.fill(7);
    passed &= Check("fill assigns every element", zero[0] == 7 && zero[1] == 7 && zero[2] == 7);
    passed &= Check("mutable iterators span exactly N elements",
                    zero.begin() == zero.data() && zero.end() == zero.data() + zero.size() &&
                    zero.end() - zero.begin() == 3);
    int sum = 0;
    for (int value : zero)
        sum += value;
    passed &= Check("range-for visits every element", sum == 21);

    const auto &constZero = zero;
    passed &= Check("const iterators span exactly N elements",
                    constZero.begin() == constZero.data() &&
                    constZero.end() == constZero.data() + constZero.size() &&
                    constZero.cbegin() == constZero.begin() &&
                    constZero.cend() == constZero.end());

    passed &= Check("at returns mutable and const references",
                    &zero.at(1) == &zero[1] && &constZero.at(2) == &constZero[2]);
    bool mutableAtThrows = false;
    bool constAtThrows = false;
    try { (void)zero.at(zero.size()); }
    catch (...) { mutableAtThrows = true; }
    try { (void)constZero.at(constZero.size()); }
    catch (...) { constAtThrows = true; }
    passed &= Check("at rejects out-of-range indices", mutableAtThrows && constAtThrows);

    Limen::LStaticArray<int, 3> swapLeft{};
    Limen::LStaticArray<int, 3> swapRight{};
    swapLeft.fill(1);
    swapRight.fill(9);
    swapLeft.Swap(swapRight);
    passed &= Check("Swap exchanges all elements without changing array sizes",
                    swapLeft.size() == 3 && swapRight.size() == 3 &&
                    swapLeft[0] == 9 && swapLeft[2] == 9 &&
                    swapRight[0] == 1 && swapRight[2] == 1);

    Limen::LStaticArray<std::unique_ptr<int>, 1> uniqueLeft{};
    Limen::LStaticArray<std::unique_ptr<int>, 1> uniqueRight{};
    uniqueLeft[0] = std::make_unique<int>(17);
    uniqueRight[0] = std::make_unique<int>(23);
    uniqueLeft.Swap(uniqueRight);
    passed &= Check("Swap supports move-only elements",
                    *uniqueLeft[0] == 23 && *uniqueRight[0] == 17);

    Limen::LStaticArray<ThrowingSwap, 1> throwingLeft{};
    Limen::LStaticArray<ThrowingSwap, 1> throwingRight{};
    bool swapThrows = false;
    try { throwingLeft.Swap(throwingRight); }
    catch (const std::runtime_error &) { swapThrows = true; }
    passed &= Check("Swap propagates element swap failure", swapThrows);

    {
        EmptyNoDefaultArray empty{};
        const EmptyNoDefaultArray &constEmpty = empty;
        passed &= Check("zero length constructs no T and has an empty range",
                        NoDefault::alive == 0 && empty.size() == 0 && empty.empty() &&
                        empty.max_size() == 0 && empty.begin() == empty.end() &&
                        constEmpty.begin() == constEmpty.end() &&
                        constEmpty.cbegin() == constEmpty.cend());
        bool emptyAtThrows = false;
        try { (void)empty.at(0); }
        catch (...) { emptyAtThrows = true; }
        passed &= Check("at rejects every index for N == 0", emptyAtThrows);
    }
    passed &= Check("zero length destroys no T", NoDefault::alive == 0);

    return passed ? 0 : 1;
}
