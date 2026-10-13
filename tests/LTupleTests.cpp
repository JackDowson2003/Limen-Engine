#include <iostream>
#include <memory>
#include <tuple>
#include <type_traits>
#include <utility>

#include "Limen/Tool/LTuple.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;
        std::cerr << name << ": failed\n";
        return false;
    }

    constexpr Limen::LTuple<int, float> kConstant{7, 2.5f};
    static_assert(Limen::Get<0>(kConstant) == 7);
    static_assert(Limen::Get<1>(kConstant) == 2.5f);
    static_assert(std::tuple_size_v<Limen::LTuple<>> == 0);
    static_assert(std::tuple_size_v<Limen::LTuple<int, float>> == 2);
    static_assert(std::is_same_v<std::tuple_element_t<1, Limen::LTuple<int, float>>, float>);
    static_assert(std::is_same_v<decltype(Limen::Get<0>(
        std::declval<const Limen::LTuple<int, float> &>())), const int &>);
    static_assert(std::is_same_v<decltype(Limen::Get<0>(
        std::declval<Limen::LTuple<int &> &&>())), int &>);
    static_assert(sizeof(Limen::LTuple<float, float>) == sizeof(float) * 2);
}

int main()
{
    bool passed = true;

    Limen::LTuple<> empty;
    (void)empty;
    Limen::LTuple<int, int> pair;
    passed &= Check("default construction initializes values",
                    pair.Get<0>() == 0 && pair.Get<1>() == 0);

    auto &[first, second] = pair;
    first = 10;
    second = 20;
    passed &= Check("structured binding modifies separate equal-typed elements",
                    pair.Get<0>() == 10 && pair.Get<1>() == 20);

    const auto &readOnly = pair;
    const auto &[left, right] = readOnly;
    passed &= Check("const structured binding reads stored values",
                    left == 10 && right == 20);

    auto mousePosition = []() -> Limen::LTuple<float, float>
    {
        return {1.5f, -2.0f};
    };
    const auto [mouseX, mouseY] = mousePosition();
    passed &= Check("implicit brace return and value binding match mouse input usage",
                    mouseX == 1.5f && mouseY == -2.0f);

    int external = 3;
    auto borrowed = Limen::TieLTuple(external);
    borrowed.Get<0>() = 9;
    passed &= Check("reference element borrows rather than copies", external == 9);

    auto owning = Limen::MakeLTuple(std::make_unique<int>(42));
    auto [movedValue] = std::move(owning);
    passed &= Check("move-only element can be moved out", movedValue && *movedValue == 42);

    auto three = Limen::MakeLTuple(1, 2.0f, 'c');
    passed &= Check("three heterogeneous elements retain their indexes",
                    Limen::Get<0>(three) == 1 && Limen::Get<1>(three) == 2.0f &&
                    Limen::Get<2>(three) == 'c');

    return passed ? 0 : 1;
}
