#include "Limen/Scene/SceneComponent.h"

#include <cmath>
#include <iostream>
#include <string_view>
#include <variant>

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

    bool Near(const float actual, const float expected)
    {
        return std::abs(actual - expected) <= 1.0e-5f;
    }

    void TestVariantKinds(TestRunner &runner)
    {
        Limen::SceneObjectComponent model;
        model.ID = Limen::GenerateSceneComponentID();
        model.LocalTransform.Location = {1.0f, 2.0f, 3.0f};
        model.Data = Limen::ModelRendererComponent{
            .ModelAssetID = {
                0x0011223344556677ULL,
                0x8899aabbccddeeffULL
            }
        };

        runner.Check(
            "model component has model kind",
            Limen::GetSceneComponentKind(model) ==
            Limen::SceneComponentKind::ModelRenderer);
        runner.Check(
            "model component stores model asset",
            std::get<Limen::ModelRendererComponent>(model.Data).
                ModelAssetID ==
            Limen::AssetID{
                0x0011223344556677ULL,
                0x8899aabbccddeeffULL
            });

        Limen::SceneObjectComponent primitive;
        primitive.ID = Limen::GenerateSceneComponentID();
        primitive.Data = Limen::PrimitiveRendererComponent{
            .Primitive = Limen::PrimitiveType::Cube
        };
        runner.Check(
            "primitive component has primitive kind",
            Limen::GetSceneComponentKind(primitive) ==
            Limen::SceneComponentKind::PrimitiveRenderer);
        runner.Check(
            "primitive component stores cube type",
            std::get<Limen::PrimitiveRendererComponent>(primitive.Data).
                Primitive == Limen::PrimitiveType::Cube);

        Limen::SceneObjectComponent light;
        light.ID = Limen::GenerateSceneComponentID();
        light.LocalTransform.Location = {4.0f, 5.0f, 6.0f};
        light.Data = Limen::PointLightComponent{
            .Color = {1.0f, 0.5f, 0.25f},
            .Intensity = 8.0f
        };
        const auto &lightData =
            std::get<Limen::PointLightComponent>(light.Data);
        runner.Check(
            "point light component has light kind",
            Limen::GetSceneComponentKind(light) ==
            Limen::SceneComponentKind::PointLight);
        runner.Check(
            "point light component stores color",
            Near(lightData.Color.x, 1.0f) &&
            Near(lightData.Color.y, 0.5f) &&
            Near(lightData.Color.z, 0.25f));
        runner.Check(
            "point light component stores intensity",
            Near(lightData.Intensity, 8.0f));
        runner.Check(
            "point light position comes from component transform",
            Near(light.LocalTransform.Location.x, 4.0f) &&
            Near(light.LocalTransform.Location.y, 5.0f) &&
            Near(light.LocalTransform.Location.z, 6.0f));
    }

    void TestComponentIDs(TestRunner &runner)
    {
        Limen::SceneComponentID previous{};
        for (int i = 0; i < 32; ++i)
        {
            const Limen::SceneComponentID generated =
                Limen::GenerateSceneComponentID();
            runner.Check(
                "generated component ID is valid",
                generated.isValid());
            runner.Check(
                "generated component IDs differ",
                i == 0 || !(generated == previous));
            previous = generated;

            const Limen::LString text =
                Limen::SceneComponentIDToString(generated);
            const std::string_view view = text;
            runner.Check(
                "component ID has UUID length",
                view.size() == 36);
            if (view.size() != 36)
                continue;

            runner.Check(
                "component ID has v4 version",
                view[14] == '4');
            runner.Check(
                "component ID has RFC 4122 variant",
                view[19] == '8' || view[19] == '9' ||
                view[19] == 'a' || view[19] == 'b');

            Limen::SceneComponentID parsed{};
            runner.Check(
                "generated component ID parses",
                Limen::TryParseSceneComponentID(view, parsed));
            runner.Check(
                "generated component ID round-trips",
                parsed == generated);
        }
    }
}

int main()
{
    TestRunner runner;
    TestVariantKinds(runner);
    TestComponentIDs(runner);
    return runner.Failures == 0 ? 0 : 1;
}
