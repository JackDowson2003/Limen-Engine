//
// Created by chenlong on 2026/9/7.
//

#include <glm/geometric.hpp>
#include "Limen/Scene/Scene.h"

#include "Limen/Core/Log.h"

namespace Limen
{
    SceneRenderObjectHandle Scene::AddRenderObject(const SceneRenderObject &renderObject)
    {
        /*
         * SceneRenderObject 是明确的“可渲染对象”，所以 Mesh 和
         * Material 都不能为空。未来普通 Entity 可以没有渲染组件，
         * 但那不属于当前 SceneRenderObject 的职责。
         */
        LM_CORE_ASSERT(renderObject.MeshResource, "Scene cannot add a render object without Mesh");

        LM_CORE_ASSERT(renderObject.MaterialResource, "Scene cannot add a render object without Material");

        /*
         * Release 构建可能关闭断言，因此仍然保留运行时保护，
         * 避免无效对象进入场景并在 SceneRenderer 中被解引用。
         */
        if (!renderObject.MeshResource || !renderObject.MaterialResource)
            return {};

        LM_CORE_ASSERT(
            m_RenderObjects.size() < SceneRenderObjectHandle::InvalidIndex,
            "Scene contains too many render objects"
        );

        if (m_RenderObjects.size() >= SceneRenderObjectHandle::InvalidIndex)
        {
            return {};
        }

        const uint32_t objectIdx = static_cast<uint32_t>(m_RenderObjects.size());

        m_RenderObjects.push_back(renderObject);

        SceneRenderObjectHandle handle;
        handle.Index = objectIdx;

        return handle;
    }

    void Scene::SetDirectionalLight(const DirectionalLight &light)
    {
        /*
         * 零向量没有方向。
         * Shader 对零向量执行 normalize() 会得到无效结果。
         */
        const float directionalLengthSquared = glm::dot(light.Direction, light.Direction);

        const bool directionIsValid = directionalLengthSquared > 0.f;

        const bool intensityIsValid = light.Intensity >= 0.f;
        LM_CORE_ASSERT(
                directionIsValid,
                "DirectionalLight direction cannot be zero"
            );

        LM_CORE_ASSERT(
            intensityIsValid,
            "DirectionalLight intensity cannot be negative"
        );

        // Release 构建仍然需要运行时保护。
        if (!directionIsValid || !intensityIsValid)
            return;

        m_DirectionalLight = light;
    }

    const DirectionalLight & Scene::GetDirectionalLight() const noexcept
    {
        return m_DirectionalLight;
    }

    bool Scene::SetRenderObjectTransform(SceneRenderObjectHandle handle, const glm::mat4 &transform)
    {
        const bool handleIsValid = handle.IsValid() && handle.Index < m_RenderObjects.size();

        LM_CORE_ASSERT(
            handleIsValid,
            "Scene cannot update invalid render object handle '{}'",
            handle.Index
        );

        if (!handleIsValid)
            return false;

        m_RenderObjects[handle.Index].Transform = transform;
        return true;
    }

    /*
     * 返回 const 引用：
     * 1. 不复制整个 vector；
     * 2. Scene 仍然拥有数组；
     * 3. SceneRenderer 只能读取，不能擅自增删对象。
     */
    const std::vector<SceneRenderObject> &Scene::GetRenderObjects() const noexcept
    {
        return m_RenderObjects;
    }
}
