#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

#include "Limen/Tool/LMap.h"
#include "Limen/Tool/LString.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    bool Matches(const Limen::LString &value, std::string_view expected)
    {
        return value.size() == expected.size() &&
               std::string_view(value.data(), value.size()) == expected &&
               value.c_str()[value.size()] == '\0';
    }

    template<typename Operation>
    bool ThrowsOutOfRange(Operation &&operation)
    {
        try
        {
            std::forward<Operation>(operation)();
        }
        catch (const std::out_of_range &)
        {
            return true;
        }
        return false;
    }

    template<typename Operation>
    bool ThrowsLengthError(Operation &&operation)
    {
        try
        {
            std::forward<Operation>(operation)();
        }
        catch (const std::length_error &)
        {
            return true;
        }
        return false;
    }

    bool TestEmptyConstructionAndCopy()
    {
        bool passed = true;
        Limen::LString empty;
        Limen::LString fromEmptyLiteral("");
        Limen::LString fromNull(static_cast<const char *>(nullptr));
        Limen::LString fromEmptyView(std::string_view{});
        Limen::LString fromZeroCount(0, 'x');
        Limen::LString copied(empty);
        Limen::LString assigned("previous");
        assigned = empty;

        passed &= Check("empty constructors and copies have a terminator",
                        Matches(empty, "") && Matches(fromEmptyLiteral, "") &&
                        Matches(fromNull, "") && Matches(fromEmptyView, "") &&
                        Matches(fromZeroCount, "") &&
                        Matches(copied, "") && Matches(assigned, ""));
        passed &= Check("empty iterators agree", empty.begin() == empty.end() &&
                        copied.cbegin() == copied.cend());

        copied.push_back('x');
        passed &= Check("copy owns independent storage",
                        Matches(copied, "x") && Matches(empty, ""));
        return passed;
    }

    bool TestMovedFromReuse()
    {
        bool passed = true;
        Limen::LString source("first");
        Limen::LString moved(std::move(source));
        passed &= Check("move construction leaves a readable empty source",
                        Matches(moved, "first") && Matches(source, "") &&
                        source.begin() == source.end() &&
                        Matches(source.substr(), ""));

        Limen::LString copiedFromMoved(source);
        source.append("again");
        passed &= Check("moved-from source can be copied and appended",
                        Matches(copiedFromMoved, "") && Matches(source, "again"));

        Limen::LString target("old");
        target = std::move(source);
        passed &= Check("move assignment leaves a readable empty source",
                        Matches(target, "again") && Matches(source, ""));
        source.erase();
        source.resize(0);
        passed &= Check("empty mutations preserve moved-from reuse", Matches(source, ""));
        source.insert(0, "reused");
        passed &= Check("move-assigned source can be inserted into",
                        Matches(source, "reused") && Matches(target, "again"));
        return passed;
    }

    bool TestSelfReference()
    {
        bool passed = true;
        Limen::LString appended("0123456789abcdef");
        appended.append(appended);
        passed &= Check("self append survives growth",
                        Matches(appended, "0123456789abcdef0123456789abcdef"));
        appended.append(appended.data() + 2, 5);
        passed &= Check("append from own substring survives overlap",
                        Matches(appended, "0123456789abcdef0123456789abcdef23456"));

        Limen::LString inserted("abcd");
        inserted.insert(2, inserted.data() + 1, 2);
        passed &= Check("insert from own substring survives shifting",
                        Matches(inserted, "abbccd"));

        Limen::LString grown("0123456789abcdef");
        grown.insert(8, grown);
        passed &= Check("self insert survives growth",
                        Matches(grown, "012345670123456789abcdef89abcdef"));

        Limen::LString assigned("0123456789abcdef");
        assigned = assigned.c_str() + 3;
        passed &= Check("assign from own substring preserves source bytes",
                        Matches(assigned, "3456789abcdef"));
        Limen::LString &self = assigned;
        assigned = self;
        assigned = std::move(self);
        passed &= Check("self copy and move assignment preserve content",
                        Matches(assigned, "3456789abcdef"));
        return passed;
    }

    bool TestEmbeddedNullAndViews()
    {
        bool passed = true;
        const char bytes[] = {'a', '\0', 'b', 'c', 'x'};
        const std::string_view view(bytes, 4);
        const Limen::LString fromView(view);
        const Limen::LString fromLength(bytes, 4);
        const Limen::LString fromStdString(std::string(bytes, 4));

        passed &= Check("bounded sources retain embedded NUL bytes",
                        Matches(fromView, view) && Matches(fromLength, view) &&
                        Matches(fromStdString, view) && fromView[1] == '\0');
        passed &= Check("string_view conversion retains full length",
                        static_cast<std::string_view>(fromView) == view);
        passed &= Check("equality and comparison inspect bytes past NUL",
                        fromView == fromLength && fromView == fromStdString &&
                        fromView != Limen::LString("a") &&
                        fromView.compare(Limen::LString("a")) > 0);
        passed &= Check("substring retains embedded NUL bytes",
                        Matches(fromView.substr(1, 2), std::string_view(bytes + 1, 2)));
        return passed;
    }

    bool TestHashingInLMap()
    {
        bool passed = true;
        const char embedded[] = {'k', '\0', 'x'};
        Limen::LString key(embedded, 3);
        Limen::LString equalKey(std::string_view(embedded, 3));
        Limen::LString distinct(embedded, 2);
        Limen::LMap<Limen::LString, int> values;

        values.emplace(key, 7);
        passed &= Check("equal LString keys have equal hashes",
                        key == equalKey &&
                        std::hash<Limen::LString>{}(key) ==
                            std::hash<Limen::LString>{}(equalKey));
        passed &= Check("LMap finds an equivalent key with embedded NUL",
                        values.contains(equalKey) && values.at(equalKey) == 7);
        values.emplace(distinct, 9);
        passed &= Check("LMap distinguishes lengths across embedded NUL",
                        values.size() == 2 && values.at(key) == 7 &&
                        values.at(distinct) == 9);
        return passed;
    }

    bool TestGrowthAndBounds()
    {
        bool passed = true;
        Limen::LString value;
        std::string expected;
        for (int i = 0; i < 512; ++i)
        {
            const char ch = static_cast<char>('a' + i % 26);
            value.push_back(ch);
            expected.push_back(ch);
        }
        passed &= Check("repeated growth preserves every byte and terminator",
                        value.capacity() >= expected.size() && Matches(value, expected));

        value.reserve(1024);
        passed &= Check("reserve preserves content", value.capacity() >= 1024 &&
                        Matches(value, expected));
        value.resize(515, 'z');
        expected.append(3, 'z');
        passed &= Check("resize fills new bytes", Matches(value, expected));
        value.resize(10);
        passed &= Check("resize down maintains terminator",
                        Matches(value, std::string_view(expected.data(), 10)));

        const Limen::LString &readOnly = value;
        const std::size_t invalid = value.size() + 1;
        passed &= Check("at rejects invalid mutable and const indices",
                        ThrowsOutOfRange([&] { (void)value.at(value.size()); }) &&
                        ThrowsOutOfRange([&] { (void)readOnly.at(readOnly.size()); }));
        passed &= Check("insert, erase and substr reject invalid positions",
                        ThrowsOutOfRange([&] { value.insert(invalid, "x"); }) &&
                        ThrowsOutOfRange([&] { value.erase(invalid); }) &&
                        ThrowsOutOfRange([&] { (void)value.substr(invalid); }));
        passed &= Check("out-of-range operations preserve content",
                        Matches(value, std::string_view(expected.data(), 10)));
        passed &= Check("impossible capacities fail before arithmetic wraps",
                        ThrowsLengthError([&] { value.reserve(Limen::LString::npos); }) &&
                        ThrowsLengthError([&] { value.append(Limen::LString::npos, 'x'); }) &&
                        Matches(value, std::string_view(expected.data(), 10)));
        return passed;
    }

    bool TestStreamExtraction()
    {
        bool passed = true;
        std::istringstream input("  alpha beta");
        Limen::LString first;
        Limen::LString second;
        input >> first;
        passed &= Check("extraction leaves the delimiter unread",
                        Matches(first, "alpha") && input.peek() == ' ');
        input >> second;
        passed &= Check("the next extraction skips the delimiter",
                        Matches(second, "beta") && input.eof() && !input.fail());

        std::istringstream limited("abcdef");
        Limen::LString prefix;
        limited >> std::setw(3) >> prefix;
        passed &= Check("extraction honors field width",
                        Matches(prefix, "abc") && limited.peek() == 'd');

        std::istringstream noSkip(" x");
        noSkip >> std::noskipws;
        Limen::LString unchanged("old");
        noSkip >> unchanged;
        passed &= Check("noskipws does not consume leading whitespace",
                        noSkip.fail() && Matches(unchanged, ""));
        return passed;
    }
}

int main()
{
    bool passed = true;
    passed &= TestEmptyConstructionAndCopy();
    passed &= TestMovedFromReuse();
    passed &= TestSelfReference();
    passed &= TestEmbeddedNullAndViews();
    passed &= TestHashingInLMap();
    passed &= TestGrowthAndBounds();
    passed &= TestStreamExtraction();
    return passed ? 0 : 1;
}
