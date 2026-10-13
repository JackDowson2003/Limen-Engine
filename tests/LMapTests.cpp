#include <cstddef>
#include <cstdint>
#include <functional>
#include <initializer_list>
#include <iostream>
#include <limits>
#include <memory>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

#include "Limen/Tool/LMap.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    struct ConstantHash
    {
        std::size_t operator()(int) const noexcept { return 1; }
    };

    struct ModuloHash
    {
        std::size_t operator()(int key) const noexcept { return static_cast<std::size_t>(key % 100); }
    };

    struct ModuloEqual
    {
        bool operator()(int left, int right) const noexcept
        {
            return left % 100 == right % 100;
        }
    };

    struct StatefulHash
    {
        int modulus;
        int seed;

        std::size_t operator()(int key) const noexcept
        {
            return std::hash<int>{}((key % modulus) ^ seed);
        }
    };

    struct StatefulEqual
    {
        int modulus;

        bool operator()(int left, int right) const noexcept
        {
            return left % modulus == right % modulus;
        }
    };

    struct alignas(64) AlignedValue
    {
        explicit AlignedValue(int initial) : value(initial) {}

        int value;
    };

    struct NonDefault
    {
        NonDefault() = delete;
        explicit NonDefault(int initial) : value(initial) {}

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

        ThrowingCopy(ThrowingCopy &&other) noexcept : value(other.value)
        {
            other.value = -1;
            ++live;
        }

        ~ThrowingCopy() { --live; }

        int value;
    };

    using StringMap = Limen::LMap<std::string, int>;
    static_assert(std::is_same_v<StringMap::key_type, std::string>);
    static_assert(std::is_same_v<StringMap::mapped_type, int>);
    static_assert(std::is_same_v<decltype(std::declval<StringMap::value_type &>().second), int>);
    static_assert(std::is_const_v<std::remove_reference_t<
        decltype(std::declval<StringMap::value_type &>().first)>>);

    bool TestBasicOperations()
    {
        bool passed = true;
        StringMap values;
        passed &= Check("default map is empty",
                        values.empty() && values.size() == 0 && values.begin() == values.end());

        auto [first, insertedFirst] = values.emplace("alpha", 1);
        passed &= Check("emplace inserts a new key",
                        insertedFirst && first->first == "alpha" && first->second == 1 &&
                        values.size() == 1);

        auto [duplicate, insertedDuplicate] = values.emplace("alpha", 99);
        passed &= Check("duplicate key keeps the original value",
                        !insertedDuplicate && duplicate->second == 1 && values.size() == 1);

        auto [inserted, wasInserted] = values.insert(StringMap::value_type{"delta", 4});
        passed &= Check("insert accepts a key-value entry",
                        wasInserted && inserted->second == 4 && values.size() == 2);

        auto [existing, didTryInsert] = values.try_emplace("alpha", 77);
        passed &= Check("try_emplace leaves an existing value unchanged",
                        !didTryInsert && existing->second == 1 && values.size() == 2);

        auto [assigned, didAssignInsert] = values.insert_or_assign("delta", 44);
        passed &= Check("insert_or_assign updates an existing key",
                        !didAssignInsert && assigned->second == 44 && values.size() == 2);
        auto [newlyAssigned, didInsert] = values.insert_or_assign("epsilon", 5);
        passed &= Check("insert_or_assign inserts a missing key",
                        didInsert && newlyAssigned->second == 5 && values.size() == 3);

        values["beta"] = 2;
        passed &= Check("operator[] inserts and updates mapped values",
                        values.size() == 4 && values.at("beta") == 2);
        int &defaultValue = values["gamma"];
        passed &= Check("operator[] value-initializes a missing value",
                        values.size() == 5 && defaultValue == 0);
        defaultValue = 3;

        passed &= Check("find and contains distinguish existing keys",
                        values.find("alpha") != values.end() &&
                        values.find("alpha")->second == 1 &&
                        values.find("missing") == values.end() &&
                        values.contains("gamma") && !values.contains("missing") &&
                        values.count("gamma") == 1 && values.count("missing") == 0);

        const StringMap &readOnly = values;
        passed &= Check("const find and at borrow stored values",
                        readOnly.find("beta") != readOnly.end() &&
                        readOnly.find("beta")->second == 2 && readOnly.at("gamma") == 3);

        bool mutableAtThrew = false;
        bool constAtThrew = false;
        try { (void)values.at("missing"); }
        catch (const std::out_of_range &) { mutableAtThrew = true; }
        try { (void)readOnly.at("missing"); }
        catch (const std::out_of_range &) { constAtThrew = true; }
        passed &= Check("at rejects a missing key without inserting it",
                        mutableAtThrew && constAtThrew && values.size() == 5);

        passed &= Check("erase returns the number of removed keys",
                        values.erase("beta") == 1 && values.erase("beta") == 0 &&
                        values.size() == 4 && !values.contains("beta"));
        auto afterErased = values.erase(StringMap::const_iterator(values.find("delta")));
        passed &= Check("erase(const_iterator) removes only the pointed entry",
                        values.size() == 3 && !values.contains("delta") &&
                        (afterErased == values.end() || afterErased->first != "delta"));
        values.clear();
        passed &= Check("clear destroys all entries", values.empty() && values.begin() == values.end());
        values.emplace("reused", 7);
        passed &= Check("map can be reused after clear",
                        values.size() == 1 && values.at("reused") == 7);
        return passed;
    }

    bool TestCollisionsAndRehash()
    {
        bool passed = true;
        Limen::LMap<int, int, ConstantHash> values;

        // 所有键具有相同哈希值；按插入和删除规则独立计算预期结果。
        for (int key = 0; key < 96; ++key)
            values.emplace(key, key * 7);
        for (int key = 0; key < 96; key += 3)
            values.erase(key);
        for (int key = 96; key < 144; ++key)
            values.emplace(key, key * 7);

        values.reserve(256);
        passed &= Check("reserve retains all colliding entries",
                        values.size() == 112);
        int *stableValue = &values.at(101);
        values.rehash(512);
        passed &= Check("rehash provides the requested bucket count",
                        values.bucket_count() >= 512);
        passed &= Check("rehash preserves references to node values",
                        stableValue == &values.at(101) && *stableValue == 707);

        bool matchesExpected = true;
        for (int key = 0; key < 144; ++key)
        {
            const auto found = values.find(key);
            const bool shouldExist = key >= 96 || key % 3 != 0;
            if ((found != values.end()) != shouldExist ||
                (shouldExist && found->second != key * 7))
            {
                std::cerr << "collision mismatch for key " << key << '\n';
                matchesExpected = false;
                break;
            }
        }
        passed &= Check("collision chain matches insertion and removal rules", matchesExpected);

        int iterated = 0;
        bool visited[144]{};
        bool eachOnce = true;
        for (const auto &[key, value] : values)
        {
            if (key < 0 || key >= 144 || visited[key] || value != key * 7)
            {
                eachOnce = false;
                break;
            }
            visited[key] = true;
            ++iterated;
        }
        passed &= Check("collision iteration visits every surviving key once",
                        eachOnce && iterated == 112);
        return passed;
    }

    bool TestCustomKeyEquality()
    {
        bool passed = true;
        Limen::LMap<int, int, ModuloHash, ModuloEqual> values;
        values.emplace(1, 10);
        auto [found, inserted] = values.emplace(101, 20);
        passed &= Check("custom key equality prevents equivalent duplicates",
                        !inserted && values.size() == 1 && found->second == 10 &&
                        values.find(201) != values.end());
        passed &= Check("erase uses custom key equality",
                        values.erase(301) == 1 && values.empty());
        return passed;
    }

    bool TestLoadFactorPolicy()
    {
        bool passed = true;
        Limen::LMap<int, int> values;
        passed &= Check("default maximum load factor is one",
                        values.max_load_factor() == 1.0f);

        values.max_load_factor(2.0f);
        for (int key = 0; key < 12; ++key)
            values.emplace(key, key * 3);
        passed &= Check("higher maximum load factor permits denser buckets",
                        values.bucket_count() == 8 && values.load_factor() == 1.5f);

        int *stableValue = &values.at(7);
        values.max_load_factor(0.5f);
        passed &= Check("lowering maximum load factor expands buckets before changing policy",
                        values.max_load_factor() == 0.5f &&
                        values.bucket_count() >= 24 && values.load_factor() <= 0.5f &&
                        stableValue == &values.at(7) && *stableValue == 21);

        values.reserve(37);
        passed &= Check("reserve converts element count using maximum load factor",
                        values.bucket_count() >= 74);
        values.rehash(1);
        passed &= Check("rehash cannot violate maximum load factor",
                        values.bucket_count() >= 24 && values.load_factor() <= 0.5f &&
                        values.at(7) == 21);

        bool rejectedInvalidFactor = true;
        for (float factor : {0.0f, -1.0f,
                             std::numeric_limits<float>::infinity(),
                             std::numeric_limits<float>::quiet_NaN()})
        {
            try { values.max_load_factor(factor); rejectedInvalidFactor = false; }
            catch (const std::invalid_argument &) { }
        }
        passed &= Check("invalid load factors leave the map unchanged",
                        rejectedInvalidFactor && values.max_load_factor() == 0.5f &&
                        values.at(7) == 21);

        bool rejectedUnrepresentableCapacity = false;
        try { values.max_load_factor(std::numeric_limits<float>::min()); }
        catch (const std::length_error &) { rejectedUnrepresentableCapacity = true; }
        passed &= Check("load factor cannot require more buckets than LArray supports",
                        rejectedUnrepresentableCapacity && values.max_load_factor() == 0.5f &&
                        values.at(7) == 21);

        const Limen::LMap<int, int> copy(values);
        passed &= Check("copy preserves maximum load factor",
                        copy.max_load_factor() == 0.5f && copy.at(7) == 21);
        Limen::LMap<int, int> assigned;
        assigned = copy;
        passed &= Check("copy assignment preserves maximum load factor",
                        assigned.max_load_factor() == 0.5f && assigned.at(7) == 21);
        Limen::LMap<int, int> moved(std::move(values));
        passed &= Check("move transfers maximum load factor and resets source policy",
                        moved.max_load_factor() == 0.5f && moved.at(7) == 21 &&
                        values.max_load_factor() == 1.0f);
        Limen::LMap<int, int> moveAssigned;
        moveAssigned = std::move(moved);
        passed &= Check("move assignment preserves maximum load factor",
                        moveAssigned.max_load_factor() == 0.5f && moveAssigned.at(7) == 21 &&
                        moved.max_load_factor() == 1.0f);
        return passed;
    }

    bool TestStatefulFunctorsAndAlignment()
    {
        bool passed = true;
        using StatefulMap = Limen::LMap<int, int, StatefulHash, StatefulEqual>;

        StatefulMap source(8, StatefulHash{10, 1}, StatefulEqual{10});
        source.emplace(1, 11);
        source.emplace(2, 22);

        StatefulMap copied(8, StatefulHash{7, 5}, StatefulEqual{7});
        copied.emplace(3, 33);
        copied = source;
        passed &= Check("copy assignment transfers hash and equality state",
                        copied.size() == 2 && copied.at(11) == 11 &&
                        copied.at(12) == 22 && !copied.contains(3));
        copied.erase(21);
        passed &= Check("copy assignment keeps source independent",
                        copied.size() == 1 && source.size() == 2 && source.contains(11));

        StatefulMap moved(8, StatefulHash{4, 9}, StatefulEqual{4});
        moved.emplace(3, 33);
        moved = std::move(source);
        passed &= Check("move assignment transfers hash and equality state",
                        moved.size() == 2 && moved.at(11) == 11 &&
                        moved.at(12) == 22 && !moved.contains(3));

        StatefulMap left(8, StatefulHash{5, 3}, StatefulEqual{5});
        StatefulMap right(8, StatefulHash{7, 8}, StatefulEqual{7});
        left.max_load_factor(0.5f);
        right.max_load_factor(2.0f);
        left.emplace(1, 100);
        right.emplace(2, 200);
        left.Swap(right);
        passed &= Check("Swap exchanges functors together with nodes",
                        left.size() == 1 && left.at(9) == 200 &&
                        right.size() == 1 && right.at(6) == 100 &&
                        left.max_load_factor() == 2.0f &&
                        right.max_load_factor() == 0.5f);

        Limen::LMap<int, AlignedValue> aligned;
        aligned.try_emplace(7, 42);
        aligned.rehash(64);
        const auto address = reinterpret_cast<std::uintptr_t>(&aligned.at(7));
        passed &= Check("mapped value keeps its requested alignment",
                        address % alignof(AlignedValue) == 0 && aligned.at(7).value == 42);
        return passed;
    }

    bool TestIteration()
    {
        bool passed = true;
        Limen::LMap<int, int> values;
        for (int key = 0; key < 32; ++key)
            values.emplace(key, key * 2);

        int visited = 0;
        bool seen[32]{};
        for (auto &[key, value] : values)
        {
            if (key >= 0 && key < 32 && !seen[key] && value == key * 2)
            {
                seen[key] = true;
                ++visited;
            }
            value += 1;
        }
        passed &= Check("mutable iteration visits every entry once", visited == 32);

        const auto &readOnly = values;
        int constVisited = 0;
        int sum = 0;
        for (auto it = readOnly.cbegin(); it != readOnly.cend(); ++it)
        {
            ++constVisited;
            sum += it->second;
        }
        passed &= Check("const iteration sees updated mapped values",
                        constVisited == 32 && sum == 32 * 32 &&
                        readOnly.begin() == readOnly.cbegin() &&
                        readOnly.end() == readOnly.cend());

        for (auto it = values.begin(); it != values.end();)
            it = values.erase(Limen::LMap<int, int>::const_iterator(it));
        passed &= Check("erase(const_iterator) supports erase while iterating",
                        values.empty() && values.begin() == values.end());
        return passed;
    }

    bool TestCopyAndMove()
    {
        bool passed = true;
        StringMap original;
        original.emplace("alpha", 1);
        original.emplace("beta", 2);

        StringMap copy(original);
        copy.at("alpha") = 10;
        passed &= Check("copy constructor owns independent values",
                        original.at("alpha") == 1 && copy.at("alpha") == 10);

        StringMap assigned;
        assigned.emplace("old", 9);
        assigned = original;
        assigned.at("beta") = 20;
        passed &= Check("copy assignment replaces previous entries",
                        assigned.size() == 2 && !assigned.contains("old") &&
                        original.at("beta") == 2 && assigned.at("beta") == 20);

        StringMap &self = assigned;
        assigned = self;
        passed &= Check("self copy assignment preserves entries",
                        assigned.size() == 2 && assigned.at("beta") == 20);

        StringMap moved(std::move(copy));
        passed &= Check("move constructor transfers entries",
                        moved.size() == 2 && moved.at("alpha") == 10);
        copy.clear();
        copy.emplace("reused", 3);
        passed &= Check("moved-from map remains reusable", copy.at("reused") == 3);

        assigned = std::move(moved);
        passed &= Check("move assignment transfers entries",
                        assigned.size() == 2 && assigned.at("alpha") == 10 &&
                        !assigned.contains("old"));
        moved.clear();
        moved.emplace("again", 4);
        passed &= Check("move-assigned source remains reusable", moved.at("again") == 4);
        return passed;
    }

    bool TestLifetimeAndNonDefaultValues()
    {
        bool passed = true;
        Tracked::live = 0;
        Tracked::constructed = 0;
        Tracked::destroyed = 0;
        {
            Limen::LMap<int, Tracked> values;
            values.emplace(1, Tracked{10});
            values.emplace(2, Tracked{20});
            passed &= Check("map owns exactly the inserted values",
                            values.size() == 2 && Tracked::live == 2);
            const int constructionsBeforeDuplicate = Tracked::constructed;
            auto [existing, inserted] = values.try_emplace(1, 99);
            passed &= Check("try_emplace skips construction for an existing key",
                            !inserted && existing->second.value == 10 &&
                            Tracked::constructed == constructionsBeforeDuplicate);
            values.rehash(64);
            passed &= Check("rehash preserves values without leaking objects",
                            Tracked::live == 2 && values.at(1).value == 10 &&
                            values.at(2).value == 20);
            values.erase(1);
            passed &= Check("erase destroys only the removed value",
                            values.size() == 1 && Tracked::live == 1);
            values.clear();
            passed &= Check("clear destroys every remaining value",
                            values.empty() && Tracked::live == 0);
        }
        passed &= Check("all constructed mapped values are destroyed",
                        Tracked::live == 0 && Tracked::constructed == Tracked::destroyed);

        Limen::LMap<int, NonDefault> noDefault;
        noDefault.try_emplace(41, 42);
        passed &= Check("try_emplace constructs a non-default-constructible value in place",
                        noDefault.at(41).value == 42);

        Limen::LMap<int, std::unique_ptr<int>> moveOnly;
        moveOnly.emplace(7, std::make_unique<int>(9));
        moveOnly.rehash(64);
        passed &= Check("move-only mapped values survive rehash",
                        moveOnly.at(7) && *moveOnly.at(7) == 9);
        return passed;
    }

    bool TestFailedCopyCleansUp()
    {
        bool passed = true;
        ThrowingCopy::live = 0;
        ThrowingCopy::copiesBeforeThrow = -1;
        {
            Limen::LMap<int, ThrowingCopy> source;
            source.emplace(1, ThrowingCopy{10});
            source.emplace(2, ThrowingCopy{20});
            source.emplace(3, ThrowingCopy{30});

            ThrowingCopy::copiesBeforeThrow = 1;
            bool copyThrew = false;
            try
            {
                Limen::LMap<int, ThrowingCopy> copy(source);
            }
            catch (const std::runtime_error &)
            {
                copyThrew = true;
            }
            passed &= Check("failed copy releases partial entries and preserves source",
                            copyThrew && source.size() == 3 && ThrowingCopy::live == 3 &&
                            source.at(1).value == 10 && source.at(2).value == 20 &&
                            source.at(3).value == 30);
        }
        ThrowingCopy::copiesBeforeThrow = -1;
        passed &= Check("exception path leaves no live values", ThrowingCopy::live == 0);
        return passed;
    }
}

int main()
{
    bool passed = true;
    passed &= TestBasicOperations();
    passed &= TestCollisionsAndRehash();
    passed &= TestCustomKeyEquality();
    passed &= TestLoadFactorPolicy();
    passed &= TestStatefulFunctorsAndAlignment();
    passed &= TestIteration();
    passed &= TestCopyAndMove();
    passed &= TestLifetimeAndNonDefaultValues();
    passed &= TestFailedCopyCleansUp();
    return passed ? 0 : 1;
}
