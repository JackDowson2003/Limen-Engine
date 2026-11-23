#include "Limen/Scene/SceneRuntime.h"

#include <cmath>
#include <iostream>
#include <string_view>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneComponent.h"

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

    bool Near(
        const glm::vec3 &actual,
        const glm::vec3 &expected)
    {
        return Near(actual.x, expected.x) &&
               Near(actual.y, expected.y) &&
               Near(actual.z, expected.z);
    }

    void TestPointLightCollection(TestRunner &runner)
    {
        Limen::Scene scene;

        Limen::PointLight legacy;
        legacy.Position = {-3.0f, 1.0f, 2.0f};
        legacy.Color = {0.25f, 0.5f, 1.0f};
        legacy.Intensity = 2.0f;
        runner.Check(
            "legacy point light is added",
            scene.AddPointLight(legacy).IsValid());

        const Limen::SceneObjectHandle lightObject =
            scene.CreateObject();
        Limen::SceneTransform rootTransform;
        rootTransform.Location = {10.0f, 20.0f, 30.0f};
        rootTransform.Rotation = glm::angleAxis(
            glm::radians(90.0f),
            glm::vec3{0.0f, 1.0f, 0.0f});
        rootTransform.Scale = {2.0f, 2.0f, 2.0f};
        runner.Check(
            "light object root transform is set",
            scene.SetObjectTransform(lightObject, rootTransform));

        Limen::SceneObjectComponent lightComponent;
        lightComponent.LocalTransform.Location = {1.0f, 0.0f, 0.0f};
        lightComponent.Data = Limen::PointLightComponent{
            .Color = {1.0f, 0.5f, 0.25f},
            .Intensity = 4.0f
        };
        runner.Check(
            "point light component is added",
            scene.AddObjectComponent(
                lightObject,
                lightComponent).isValid());

        Limen::SceneObjectComponent modelComponent;
        modelComponent.Data = Limen::ModelRendererComponent{
            .ModelAssetID = {
                0x0011223344556677ULL,
                0x8899aabbccddeeffULL
            }
        };
        runner.Check(
            "non-light model component is added",
            scene.AddObjectComponent(
                lightObject,
                modelComponent).isValid());

        Limen::LArray<Limen::PointLight> lights =
            Limen::CollectRuntimePointLights(scene);
        runner.Check(
            "legacy and component lights are collected",
            lights.getSize() == 2);
        if (lights.getSize() != 2)
            return;

        runner.Check(
            "legacy light is preserved",
            Near(lights[0].Position, legacy.Position) &&
            Near(lights[0].Color, legacy.Color) &&
            Near(lights[0].Intensity, legacy.Intensity));

        runner.Check(
            "component light combines root and local transforms",
            Near(lights[1].Position, {10.0f, 20.0f, 28.0f}));
        runner.Check(
            "component light preserves color",
            Near(lights[1].Color, {1.0f, 0.5f, 0.25f}));
        runner.Check(
            "component light preserves intensity",
            Near(lights[1].Intensity, 4.0f));

        runner.Check(
            "light object can be destroyed",
            scene.DestroyObject(lightObject));
        lights = Limen::CollectRuntimePointLights(scene);
        runner.Check(
            "destroyed component light stops rendering",
            lights.getSize() == 1 &&
            Near(lights[0].Position, legacy.Position));
    }
}

int main()
{
    TestRunner runner;
    TestPointLightCollection(runner);
    return runner.Failures == 0 ? 0 : 1;
}
