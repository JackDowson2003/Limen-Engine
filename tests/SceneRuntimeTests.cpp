#include "Limen/Scene/SceneRuntime.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <string_view>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>

#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneComponent.h"
#include "Limen/Scene/SceneSerializer.h"

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

    void TestSceneInstantiation(TestRunner &runner)
    {
        Limen::SceneDocument document;
        document.DisplayName = "场景名称";
        document.Ambient.Intensity = 0.25f;

        Limen::SceneObjectAuthoringData object;
        object.ID = {1, 2};
        object.Label = "对象名称";
        object.Transform.Location = {10.0f, 20.0f, 30.0f};
        object.Transform.Scale = {2.0f, 3.0f, 4.0f};

        Limen::SceneObjectComponent cube;
        cube.ID = {3, 4};
        cube.LocalTransform.Location = {1.0f, 2.0f, 3.0f};
        cube.LocalTransform.Scale = {2.0f, 1.0f, 3.0f};
        cube.Data = Limen::PrimitiveRendererComponent{};
        object.Components.push_back(cube);

        Limen::SceneObjectComponent light;
        light.ID = {5, 6};
        light.LocalTransform.Location = {0.0f, 2.0f, 0.0f};
        light.Data = Limen::PointLightComponent{};
        object.Components.push_back(light);

        Limen::SceneObjectComponent model;
        model.ID = {7, 8};
        model.LocalTransform.Location = {-2.0f, 1.0f, 0.0f};
        model.Data = Limen::ModelRendererComponent{.ModelAssetID = {9, 10}};
        object.Components.push_back(model);
        document.Objects.push_back(object);

        // 与 SceneObjectTests 一样，这些资源句柄只用于检查挂载和传递。
        const Limen::Ref<Limen::Mesh> mesh(
            reinterpret_cast<Limen::Mesh *>(std::uintptr_t{1}), [](Limen::Mesh *) {});
        const Limen::Ref<Limen::Material> material(
            reinterpret_cast<Limen::Material *>(std::uintptr_t{2}), [](Limen::Material *) {});
        glm::mat4 partToComponent{1.0f};
        partToComponent[3] = {1.0f, 0.0f, 1.0f, 1.0f};

        const auto resolve = [&](const Limen::SceneObjectComponent &component,
                                 Limen::LArray<Limen::SceneRenderObject> &parts,
                                 Limen::SceneFileError &)
        {
            const bool isRenderable = component.ID == cube.ID || component.ID == model.ID;
            runner.Check("only renderable components reach the resolver", isRenderable);
            if (!isRenderable)
                return false;

            runner.Check("resolver receives an empty per-component output", parts.empty());
            parts.push_back({mesh, material, partToComponent});
            return true;
        };

        Limen::Scope<Limen::Scene> scene;
        Limen::SceneFileError error{.Code = Limen::SceneFileErrorCode::InvalidField};
        const bool loaded = Limen::TryInstantiateScene(document, resolve, scene, error);
        runner.Check("successful instantiation publishes a scene", loaded && scene);
        runner.Check("successful instantiation clears the error", error.Code == Limen::SceneFileErrorCode::None);
        if (!loaded || !scene)
            return;

        Limen::SceneObjectHandle restored;
        const bool found = scene->TryFindObject(object.ID, restored);
        runner.Check("instantiation preserves the object ID", found);
        Limen::LString label;
        runner.Check("instantiation preserves the object label",
                     found && scene->TryGetObjectLabel(restored, label) && label == object.Label);
        runner.Check("instantiation restores the ambient light",
                     Near(scene->GetAmbientLight().Intensity, document.Ambient.Intensity));

        int partCount = 0;
        int cubePartCount = 0;
        int modelPartCount = 0;
        scene->ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &handle, const Limen::SceneObjectID &id,
                const Limen::Ref<Limen::Mesh> &drawMesh, const Limen::Ref<Limen::Material> &drawMaterial,
                const glm::mat4 &world)
            {
                ++partCount;
                runner.Check("instantiated parts use the restored object and resolved resources",
                             handle == restored && id == object.ID && drawMesh == mesh && drawMaterial == material);
                const auto worldPosition = glm::vec3{world[3]};
                if (Near(worldPosition, {16.0f, 26.0f, 54.0f}))
                    ++cubePartCount;
                if (Near(worldPosition, {8.0f, 23.0f, 34.0f}))
                    ++modelPartCount;
            });
        runner.Check("both render components combine part, component and root transforms",
                     partCount == 2 && cubePartCount == 1 && modelPartCount == 1);

        const auto lights = Limen::CollectRuntimePointLights(*scene);
        runner.Check("instantiation restores the point-light component",
                     lights.getSize() == 1 && Near(lights[0].Position, {10.0f, 26.0f, 30.0f}));

        const auto failOnModel = [&](const Limen::SceneObjectComponent &component,
                                     Limen::LArray<Limen::SceneRenderObject> &parts,
                                     Limen::SceneFileError &componentError)
        {
            if (component.ID != model.ID)
                return resolve(component, parts, componentError);

            componentError = {
                .Code = Limen::SceneFileErrorCode::FileAccess,
                .Key = "asset_id",
                .Message = "model asset is unavailable"
            };
            return false;
        };

        const auto *activeScene = scene.get();
        const bool reloaded = Limen::TryInstantiateScene(document, failOnModel, scene, error);
        runner.Check("failed resource resolution rejects instantiation", !reloaded);
        runner.Check("failed instantiation keeps the active scene", scene.get() == activeScene);
        runner.Check("failed instantiation reports the exact component and resolver error",
                     error.Code == Limen::SceneFileErrorCode::FileAccess &&
                     error.Key == "objects[0].components[2].asset_id" &&
                     error.Message == "model asset is unavailable");
    }
}

int main()
{
    TestRunner runner;
    TestPointLightCollection(runner);
    TestSceneInstantiation(runner);
    return runner.Failures == 0 ? 0 : 1;
}
