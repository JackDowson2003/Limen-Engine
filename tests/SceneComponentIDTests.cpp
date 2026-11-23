#include "Limen/Scene/Scene.h"

#include <iostream>
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

    void TestRoundTrip(TestRunner &runner)
    {
        constexpr std::string_view canonical =
            "ffeeddcc-bbaa-4998-8776-554433221100";
        constexpr Limen::SceneComponentID expected{
            0xffeeddccbbaa4998ULL,
            0x8776554433221100ULL
        };

        Limen::SceneComponentID parsed{};
        runner.Check(
            "known SceneComponentID parses",
            Limen::TryParseSceneComponentID(canonical, parsed));
        runner.Check(
            "parsed SceneComponentID preserves both halves",
            parsed == expected);

        const Limen::LString formatted =
            Limen::SceneComponentIDToString(expected);
        runner.Check(
            "SceneComponentID formats canonical lowercase UUID",
            static_cast<std::string_view>(formatted) == canonical);

        Limen::SceneComponentID roundTrip{};
        runner.Check(
            "formatted SceneComponentID parses",
            Limen::TryParseSceneComponentID(
                static_cast<std::string_view>(formatted),
                roundTrip));
        runner.Check(
            "SceneComponentID round-trips",
            roundTrip == expected);
    }

    void TestInvalidValues(TestRunner &runner)
    {
        constexpr std::string_view invalidValues[] = {
            "",
            "ffeeddcc-bbaa-4998-8776-55443322110",
            "ffeeddcc_bbaa-4998-8776-554433221100",
            "ffeeddcc-bbaa-4998-8776-55443322110g",
            "FFEEDDCC-BBAA-4998-8776-554433221100",
            "00000000-0000-0000-0000-000000000000",
        };

        constexpr Limen::SceneComponentID sentinel{
            0x0123456789abcdefULL,
            0xfedcba9876543210ULL
        };
        for (const std::string_view value: invalidValues)
        {
            Limen::SceneComponentID output = sentinel;
            runner.Check(
                "invalid SceneComponentID is rejected",
                !Limen::TryParseSceneComponentID(value, output));
            runner.Check(
                "invalid SceneComponentID preserves output",
                output == sentinel);
        }

        const Limen::LString nilText =
            Limen::SceneComponentIDToString({});
        runner.Check(
            "nil SceneComponentID formats canonically",
            static_cast<std::string_view>(nilText) ==
            "00000000-0000-0000-0000-000000000000");
    }
}

int main()
{
    TestRunner runner;
    TestRoundTrip(runner);
    TestInvalidValues(runner);
    return runner.Failures == 0 ? 0 : 1;
}
