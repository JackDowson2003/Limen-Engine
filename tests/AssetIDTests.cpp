#include "Limen/Asset/AssetID.h"

#include <iostream>
#include <string>
#include <string_view>

namespace
{
    struct TestRunner
    {
        int Failures = 0;

        void Check(const std::string_view name, const bool passed)
        {
            if (passed)
                return;

            ++Failures;
            std::cerr << "FAIL: " << name << '\n';
        }
    };

    void TestKnownValue(TestRunner& runner)
    {
        constexpr std::string_view canonical = "00112233-4455-6677-8899-aabbccddeeff";
        constexpr Limen::AssetID expected{0x0011223344556677ULL, 0x8899aabbccddeeffULL};

        Limen::AssetID parsed{};
        runner.Check("parse known lowercase ID", Limen::TryParseAssetID(canonical, parsed));
        runner.Check("parse preserves high and low halves", parsed == expected);

        const auto formatted = Limen::AssetIDToString(expected);
        runner.Check("format uses canonical lowercase groups",
                     static_cast<std::string_view>(formatted) == canonical);

        Limen::AssetID roundTrip{};
        runner.Check("parse formatted ID",
                     Limen::TryParseAssetID(static_cast<std::string_view>(formatted), roundTrip));
        runner.Check("formatted ID round-trips", roundTrip == expected);
    }

    void TestInvalidText(TestRunner& runner)
    {
        struct InvalidCase
        {
            std::string_view Name;
            std::string_view Text;
        };

        constexpr InvalidCase cases[] = {
            {"empty", ""},
            {"short", "00112233-4455-6677-8899-aabbccddeef"},
            {"long", "00112233-4455-6677-8899-aabbccddeeff0"},
            {"wrong separator", "00112233_4455-6677-8899-aabbccddeeff"},
            {"uppercase", "00112233-4455-6677-8899-aAbbccddeeff"},
            {"non-hex digit", "00112233-4455-6677-8899-aabbccddeefg"},
            {"nil ID", "00000000-0000-0000-0000-000000000000"},
        };

        // 失败不能把调用者此前持有的有效身份清空或部分覆盖。
        constexpr Limen::AssetID sentinel{0x123456789abcdef0ULL, 0x0fedcba987654321ULL};
        for (const auto& testCase : cases)
        {
            auto output = sentinel;
            const std::string label(testCase.Name);
            runner.Check(label + " rejected", !Limen::TryParseAssetID(testCase.Text, output));
            runner.Check(label + " keeps output", output == sentinel);
        }
    }

    void TestGeneratedIDs(TestRunner& runner)
    {
        for (int i = 0; i < 32; ++i)
        {
            const auto generated = Limen::GenerateAssetID();
            runner.Check("generated ID is valid", generated.isValid());

            const auto text = Limen::AssetIDToString(generated);
            runner.Check("generated ID has canonical length", text.size() == 36);
            if (text.size() != 36)
                continue;

            runner.Check("generated ID has v4 marker", text[14] == '4');
            runner.Check("generated ID has RFC 4122 variant",
                         text[19] == '8' || text[19] == '9' ||
                         text[19] == 'a' || text[19] == 'b');

            Limen::AssetID parsed{};
            runner.Check("generated ID parses",
                         Limen::TryParseAssetID(static_cast<std::string_view>(text), parsed));
            runner.Check("generated ID round-trips", parsed == generated);
        }
    }
}

int main()
{
    TestRunner runner;
    TestKnownValue(runner);
    TestInvalidText(runner);
    TestGeneratedIDs(runner);
    return runner.Failures == 0 ? 0 : 1;
}
