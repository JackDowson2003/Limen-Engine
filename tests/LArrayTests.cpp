#include <cstdint>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

#include "Limen/Tool/LArray.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    struct NonDefault
    {
        explicit NonDefault(int initial) : value(initial) {}

        NonDefault() = delete;

        int value;
    };

    struct Tracked
    {
        static inline int live = 0;
        static inline int constructed = 0;
        static inline int destroyed = 0;

        explicit Tracked(int initial) : value(initial)
        {
            ++live;
            ++constructed;
        }

        Tracked(const Tracked &other) : value(other.value)
        {
            ++live;
            ++constructed;
        }

        Tracked(Tracked &&other) noexcept : value(other.value)
        {
            other.value = -1;
            ++live;
            ++constructed;
        }

        Tracked &operator=(const Tracked &) = delete;
        Tracked &operator=(Tracked &&) = delete;

        ~Tracked()
        {
            --live;
            ++destroyed;
        }

        int value;
    };

    struct ThrowingCopy
    {
        static inline int live = 0;
        static inline int copiesBeforeThrow = -1;

        explicit ThrowingCopy(int initial) : value(initial) { ++live; }

        ThrowingCopy(const ThrowingCopy &other) : value(other.value)
        {
            if (copiesBeforeThrow == 0)
                throw std::runtime_error("copy failed");
            if (copiesBeforeThrow > 0)
                --copiesBeforeThrow;
            ++live;
        }

        // 让扩容选择可抛异常的复制，以验证失败后旧数组仍保持完整。
        ThrowingCopy(ThrowingCopy &&other) noexcept(false) : value(other.value)
        {
            other.value = -1;
            ++live;
        }

        ~ThrowingCopy() { --live; }

        int value;
    };

    bool TestConstructionAndIteration()
    {
        bool passed = true;
        Limen::LArray<int> empty;
        passed &= Check("default array is empty", empty.getSize() == 0 && empty.getCapacity() == 0);
        passed &= Check("default iterators agree", empty.begin() == empty.end());
        passed &= Check("default reverse iterators agree", empty.rbegin() == empty.rend());

        Limen::LArray<int> zero(0);
        passed &= Check("zero-length array is empty", zero.getSize() == 0 && zero.getCapacity() == 0);

        Limen::LArray<int> sized(3);
        passed &= Check("sized array contains value-initialized elements",
                        sized.getSize() == 3 && sized[0] == 0 && sized[1] == 0 && sized[2] == 0);

        Limen::LArray<int> filled(3, 7);
        passed &= Check("filled array contains supplied value",
                        filled.getSize() == 3 && filled[0] == 7 && filled[1] == 7 && filled[2] == 7);

        const Limen::LArray<int> listed{1, 2, 3};
        passed &= Check("initializer list and const indexing",
                        listed.getSize() == 3 && listed[0] == 1 && listed[1] == 2 && listed[2] == 3);
        passed &= Check("getData borrows contiguous storage",
                        listed.getData() == listed.begin() && listed.getData()[2] == 3);

        int forwardSum = 0;
        for (const int value : listed)
            forwardSum += value;
        passed &= Check("const forward iteration", forwardSum == 6);

        auto reverse = listed.crbegin();
        passed &= Check("const reverse iteration begins at last element",
                        reverse != listed.crend() && *reverse == 3);
        return passed;
    }

    bool TestGrowthAndSelfReference()
    {
        bool passed = true;
        Limen::LArray<int> numbers;
        for (int i = 0; i < 128; ++i)
            numbers.push_back(i * 3);

        bool valuesPreserved = numbers.getSize() == 128 && numbers.getCapacity() >= 128;
        for (std::uint32_t i = 0; i < numbers.getSize(); ++i)
            valuesPreserved &= numbers[i] == static_cast<int>(i) * 3;
        passed &= Check("multiple growth steps preserve values", valuesPreserved);
        passed &= Check("multiple growth steps preserve values", valuesPreserved);

        // 在这里加入 reserve 测试
        Limen::LArray<int> reserved{11, 22, 33};
        reserved.reserve(16);
        passed &= Check("reserve preserves trivial elements",
                        reserved.getSize() == 3 &&
                        reserved.getCapacity() >= 16 &&
                        reserved[0] == 11 &&
                        reserved[1] == 22 &&
                        reserved[2] == 33);

        Limen::LArray<std::string> words;
        words.emplace_back("first");
        while (words.getSize() < words.getCapacity())
            words.emplace_back("filler");
        words.push_back(words[0]);
        passed &= Check("push_back of own element survives growth", words[words.getSize() - 1] == "first");

        while (words.getSize() < words.getCapacity())
            words.emplace_back("filler");
        words.emplace_back(words[0]);
        passed &= Check("emplace_back of own element survives growth", words[words.getSize() - 1] == "first");
        return passed;
    }

    bool TestCopyAndMove()
    {
        bool passed = true;
        Limen::LArray<std::string> original{"alpha", "beta"};
        Limen::LArray<std::string> copy(original);
        copy[0] = "changed";
        passed &= Check("copy constructor makes an independent array",
                        original[0] == "alpha" && copy[0] == "changed");

        Limen::LArray<std::string> assigned{"old"};
        assigned = original;
        assigned[1] = "assigned";
        passed &= Check("copy assignment makes an independent array",
                        original[1] == "beta" && assigned[1] == "assigned");

        Limen::LArray<std::string> &self = assigned;
        assigned = self;
        passed &= Check("self copy assignment keeps values",
                        assigned.getSize() == 2 && assigned[0] == "alpha" && assigned[1] == "assigned");

        Limen::LArray<std::string> empty;
        assigned = empty;
        passed &= Check("copy assignment from empty clears elements", assigned.getSize() == 0);

        Limen::LArray<std::string> moved(std::move(copy));
        passed &= Check("move constructor transfers elements",
                        moved.getSize() == 2 && moved[0] == "changed" && copy.getSize() == 0);

        assigned = std::move(moved);
        passed &= Check("move assignment transfers elements",
                        assigned.getSize() == 2 && assigned[0] == "changed" && moved.getSize() == 0);

        Limen::LArray<std::unique_ptr<int>> moveOnly;
        moveOnly.push_back(std::make_unique<int>(4));
        moveOnly.emplace_back(std::make_unique<int>(5));
        passed &= Check("rvalue insertion supports move-only elements",
                        moveOnly.getSize() == 2 && *moveOnly[0] == 4 && *moveOnly[1] == 5);
        return passed;
    }

    bool TestLifetimeAndCapacity()
    {
        bool passed = true;
        Tracked::live = 0;
        Tracked::constructed = 0;
        Tracked::destroyed = 0;

        {
            Limen::LArray<Tracked> values;
            values.reserve(8);
            passed &= Check("reserve does not construct elements",
                            values.getSize() == 0 && values.getCapacity() >= 8 && Tracked::live == 0);

            values.emplace_back(10);
            values.emplace_back(20);
            passed &= Check("emplace constructs exactly the inserted elements",
                            values.getSize() == 2 && Tracked::live == 2 &&
                            values[0].value == 10 && values[1].value == 20);

            values.reserve(32);
            passed &= Check("reserve preserves values and live object count",
                            values.getSize() == 2 && values.getCapacity() >= 32 &&
                            Tracked::live == 2 && values[0].value == 10 && values[1].value == 20);

            values.pop_back();
            passed &= Check("pop_back destroys only the last element",
                            values.getSize() == 1 && Tracked::live == 1 && values[0].value == 10);

            values.pop_back();
            bool emptyPopThrew = false;
            try
            {
                values.pop_back();
            }
            catch (const std::out_of_range &)
            {
                emptyPopThrew = true;
            }
            passed &= Check("pop_back rejects an empty array", emptyPopThrew && values.getSize() == 0);

            values.clear();
            passed &= Check("clear destroys live elements", values.getSize() == 0 && Tracked::live == 0);
            values.emplace_back(30);
            passed &= Check("array can be reused after clear",
                            values.getSize() == 1 && values[0].value == 30 && Tracked::live == 1);
        }

        passed &= Check("destruction balances all constructions",
                        Tracked::live == 0 && Tracked::constructed == Tracked::destroyed);

        Limen::LArray<NonDefault> nonDefault;
        nonDefault.emplace_back(42);
        passed &= Check("emplace accepts non-default-constructible elements",
                        nonDefault.getSize() == 1 && nonDefault[0].value == 42);
        return passed;
    }

    bool TestExceptionCleanup()
    {
        bool passed = true;
        ThrowingCopy::live = 0;
        ThrowingCopy::copiesBeforeThrow = -1;

        {
            Limen::LArray<ThrowingCopy> values;
            values.reserve(2);
            values.emplace_back(10);
            values.emplace_back(20);

            ThrowingCopy::copiesBeforeThrow = 1;
            bool reserveThrew = false;
            try
            {
                values.reserve(4);
            }
            catch (const std::runtime_error &)
            {
                reserveThrew = true;
            }
            passed &= Check("failed reserve releases temporary elements",
                            reserveThrew && values.getSize() == 2 && values.getCapacity() == 2 &&
                            values[0].value == 10 && values[1].value == 20 && ThrowingCopy::live == 2);

            ThrowingCopy::copiesBeforeThrow = 0;
            bool appendThrew = false;
            try
            {
                values.emplace_back(30);
            }
            catch (const std::runtime_error &)
            {
                appendThrew = true;
            }
            passed &= Check("failed growth releases appended element",
                            appendThrew && values.getSize() == 2 && values.getCapacity() == 2 &&
                            values[0].value == 10 && values[1].value == 20 && ThrowingCopy::live == 2);

            ThrowingCopy::copiesBeforeThrow = 1;
            bool copyThrew = false;
            try
            {
                Limen::LArray<ThrowingCopy> copy(values);
            }
            catch (const std::runtime_error &)
            {
                copyThrew = true;
            }
            passed &= Check("failed copy constructor releases partial array",
                            copyThrew && values.getSize() == 2 && ThrowingCopy::live == 2);

            ThrowingCopy::copiesBeforeThrow = 1;
            bool fillThrew = false;
            try
            {
                Limen::LArray<ThrowingCopy> filled(2, values[0]);
            }
            catch (const std::runtime_error &)
            {
                fillThrew = true;
            }
            passed &= Check("failed fill constructor releases partial array",
                            fillThrew && values.getSize() == 2 && ThrowingCopy::live == 2);
        }

        ThrowingCopy::copiesBeforeThrow = -1;
        passed &= Check("exception tests leave no live elements", ThrowingCopy::live == 0);
        return passed;
    }
}

int main()
{
    bool passed = true;
    passed &= TestConstructionAndIteration();
    passed &= TestGrowthAndSelfReference();
    passed &= TestCopyAndMove();
    passed &= TestLifetimeAndCapacity();
    passed &= TestExceptionCleanup();
    return passed ? 0 : 1;
}
