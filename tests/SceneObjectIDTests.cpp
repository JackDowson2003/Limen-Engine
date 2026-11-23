#include "Limen/Scene/Scene.h"

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

    void TestKnownValue(TestRunner &runner)
    {
        constexpr std::string_view canonical =
            "00112233-4455-6677-8899-aabbccddeeff";
        constexpr Limen::SceneObjectID expected{
            0x0011223344556677ULL,
            0x8899aabbccddeeffULL
        };

        Limen::SceneObjectID parsed{};
        runner.Check(
            "known SceneObjectID parses",
            Limen::TryParseSceneObjectID(canonical, parsed));
        runner.Check(
            "parsed SceneObjectID preserves both halves",
            parsed == expected);

        const Limen::LString formatted =
            Limen::SceneObjectIDToString(expected);
        runner.Check(
            "SceneObjectID uses canonical lowercase UUID",
            static_cast<std::string_view>(formatted) == canonical);

        Limen::SceneObjectID roundTrip{};
        runner.Check(
            "formatted SceneObjectID parses",
            Limen::TryParseSceneObjectID(
                static_cast<std::string_view>(formatted),
                roundTrip));
        runner.Check(
            "SceneObjectID round-trips",
            roundTrip == expected);
    }

    void TestNilValue(TestRunner &runner)
    {
        constexpr std::string_view nil =
            "00000000-0000-0000-0000-000000000000";
        const Limen::LString formatted =
            Limen::SceneObjectIDToString({});
        runner.Check(
            "nil SceneObjectID formats canonically",
            static_cast<std::string_view>(formatted) == nil);

        constexpr Limen::SceneObjectID sentinel{
            0x0123456789abcdefULL,
            0xfedcba9876543210ULL
        };
        Limen::SceneObjectID output = sentinel;
        runner.Check(
            "nil SceneObjectID is rejected",
            !Limen::TryParseSceneObjectID(nil, output));
        runner.Check(
            "nil parse preserves output",
            output == sentinel);
    }

    void TestInvalidText(TestRunner &runner)
    {
        struct InvalidCase
        {
            std::string_view Name;
            std::string_view Text;
        };

        constexpr InvalidCase cases[] = {
            {"empty", ""},
            {
                "short",
                "00112233-4455-6677-8899-aabbccddeef"
            },
            {
                "long",
                "00112233-4455-6677-8899-aabbccddeeff0"
            },
            {
                "wrong separator",
                "00112233_4455-6677-8899-aabbccddeeff"
            },
            {
                "uppercase",
                "00112233-4455-6677-8899-aAbbccddeeff"
            },
            {
                "non-hex digit",
                "00112233-4455-6677-8899-aabbccddeefg"
            },
            {
                "missing separators",
                "00112233445566778899aabbccddeeff"
            },
        };

        constexpr Limen::SceneObjectID sentinel{
            0x123456789abcdef0ULL,
            0x0fedcba987654321ULL
        };
        for (const InvalidCase &testCase: cases)
        {
            Limen::SceneObjectID output = sentinel;
            const std::string label(testCase.Name);
            runner.Check(
                label + " rejected",
                !Limen::TryParseSceneObjectID(
                    testCase.Text,
                    output));
            runner.Check(
                label + " keeps output",
                output == sentinel);
        }
    }
}

int main()
{
    TestRunner runner;
    TestKnownValue(runner);
    TestNilValue(runner);
    TestInvalidText(runner);
    return runner.Failures == 0 ? 0 : 1;
}
