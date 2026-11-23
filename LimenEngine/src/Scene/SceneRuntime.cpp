//
// Created by chenlong on 2026/11/23.
//
#include "Limen/Scene/SceneRuntime.h"

#include <variant>

#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneComponent.h"

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
}
