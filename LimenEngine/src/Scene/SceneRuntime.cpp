//
// Created by chenlong on 2026/11/23.
//
#include "Limen/Scene/SceneRuntime.h"

#include <variant>
#include <cstdint>
#include <utility>

#include "Limen/Core/Core.h"
#include "Limen/Core/Log.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneComponent.h"
#include "Limen/Scene/SceneSerializer.h"

namespace Limen
{
    LArray<PointLight> CollectRuntimePointLights(const Scene &scene)
    {
        LArray<PointLight> result;

        // we'll copy the point-light in this version
        for (const auto &legacyLight: scene.GetPointLights())
            result.push_back(legacyLight);

        scene.ForEachObject(
            [&](const SceneObjectHandle &handle, const SceneObjectID &)
            {
                SceneTransform objectTransform;
                if (!scene.TryGetObjectAuthoringTransform(handle, objectTransform))
                    return;
                scene.ForEachObjectComponent(
                    handle,
                    [&](const SceneObjectComponent &component)
                    {
                        const auto *light = std::get_if<PointLightComponent>(&component.Data);
                        if (!light)
                            return;

                        /*
                         * 点光源位置来自对象根 TRS 与组件局部 TRS。
                         * 当前只取最终世界矩阵的平移分量。
                         */
                        const glm::mat4 worldTransform =
                                objectTransform.ToMatrix() * component.LocalTransform.ToMatrix();

                        result.push_back(PointLight{
                            .Position = glm::vec3{worldTransform[3]},
                            .Color = light->Color,
                            .Intensity = light->Intensity
                        });
                    }
                );
            }
        );
        return result;
    }

    bool TryInstantiateScene(
        const SceneDocument &document,
        const SceneRenderPartsResolver &resolveParts,
        Scope<Scene> &outScene, SceneFileError &error)
    {
        if (!ValidateSceneDocument(document, error))
        {
            return false;
        }

        // 创建候选的场景
        Scope<Scene> scene = CreateScope<Scene>();
        scene->SetAmbientLight(document.Ambient);
        scene->SetDirectionalLight(document.DirectionLight);
        for (std::uint32_t i = 0; i < document.Objects.getSize(); ++i)
        {
            // 持久化快照
            const SceneObjectAuthoringData &object = document.Objects[i];
            const std::string objectPath = "objects[" + std::to_string(i) + "]";

            // 从场景中恢复快照 只有ObjectID Label SceneTransform Components
            const auto handle = scene->Restore(object);
            if (!handle.IsValid())
            {
                error =
                {
                    .Code = SceneFileErrorCode::InstantiatedFailed,
                    .Key = LString(objectPath),
                    .Message = "无法恢复场景对象"
                };
                return false;
            }

            const auto &components = object.Components;
            LArray<SceneRenderObject> renderObjects;

            for (std::uint32_t j = 0; j < components.getSize(); ++j)
            {
                const auto &component = components[j];

                // 点光源由 CollectRuntimePointLights 单独收集
                if (GetSceneComponentKind(component) == SceneComponentKind::PointLight)
                    continue;

                LArray<SceneRenderObject> componentParts;
                SceneFileError componentError{};
                if (!resolveParts || !resolveParts(component, componentParts, componentError))
                {
                    // 把 resolver 给出的相对字段补成完整的场景文档路径。
                    LString key(objectPath + ".components[" + std::to_string(j) + "]");
                    if (!componentError.Key.empty())
                    {
                        key += '.';
                        key += componentError.Key;
                    }
                    componentError.Key = std::move(key);

                    if (componentError.Code == SceneFileErrorCode::None)
                        componentError.Code = SceneFileErrorCode::InstantiatedFailed;
                    if (componentError.Message.empty())
                        componentError.Message = "无法解析组件的绘制资源";

                    error = std::move(componentError);
                    return false;
                }

                // resolver 返回组件空间的绘制项，挂进 Scene 前转到对象空间。
                const auto componentToObject = component.LocalTransform.ToMatrix();
                for (auto &part : componentParts)
                {
                    part.Transform = componentToObject * part.Transform;
                    renderObjects.push_back(std::move(part));
                }
            }

            if (!scene->SetObjectRenderParts(handle,std::move(renderObjects)))
            {
                error = {
                    .Code = SceneFileErrorCode::InstantiatedFailed,
                    .Key = LString(objectPath),
                    .Message = "无法为场景对象挂载绘制项"
                };
                return false;
            }
        }
        outScene = std::move(scene);
        error = {};
        return true;
    }
}
