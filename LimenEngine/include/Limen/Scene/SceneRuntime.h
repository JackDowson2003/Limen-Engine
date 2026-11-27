//
// Created by chenlong on 2026/11/23.
//

#pragma once

#include <functional>
#include "Limen/Core/Core.h"
#include "Limen/Scene/Light.h"
#include "Limen/Tool/LArray.h"

namespace Limen
{
    class Scene;

    /*
     * 把 Scene 的 Authoring 数据解析为当前 Renderer 可消费的数据。
     *
     * 过渡期同时保留：
     * - Scene 级旧 PointLight；
     * - SceneObjectComponent 中的 PointLightComponent。
     *
     * 全部调用方迁移到组件后，再删除旧 Scene::m_PointLights。
     */
    [[nodiscard]]
    LIMEN_API LArray<PointLight> CollectRuntimePointLights(
        const Scene &scene
    );

    struct SceneDocument;
    struct SceneFileError;
    struct SceneObjectComponent;
    struct SceneRenderObject;

    // 返回组件空间中的渲染组件
    // 失败时可填写错误码、原因和相对于组件的字段路径
    using SceneRenderPartsResolver = std::function<bool(
        const SceneObjectComponent &component,
        LArray<SceneRenderObject> &outParts,
        SceneFileError &error)>;

    [[nodiscard]] LIMEN_API bool TryInstantiateScene(
        const SceneDocument &document,
        const SceneRenderPartsResolver &resolveParts,
        Scope<Scene> &outScene,
        SceneFileError &error);
}
