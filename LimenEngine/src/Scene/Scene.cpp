//
// Created by chenlong on 2026/9/7.
//

#include <glm/geometric.hpp>
#include <cstddef>
#include <cstdint>
#include <random>

#include "Limen/Scene/Scene.h"
#include "Limen/Core/Log.h"

namespace Limen
{
    namespace
    {
        SceneObjectID GenerateSceneObjectID()
        {
            std::uint8_t bytes[16]{};
            std::random_device device;
            std::uniform_int_distribution<unsigned int> byte(0, 255);

            for (auto &value: bytes)
                value = static_cast<std::uint8_t>(byte(device));

            // 与 AssetID 一样使用 UUID v4 的版本和 variant 位，但类型保持独立。
            bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0f) | 0x40);
            bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3f) | 0x80);

            SceneObjectID id{};
            for (std::size_t i = 0; i < 8; ++i)
                id.High = (id.High << 8) | bytes[i];
            for (std::size_t i = 8; i < 16; ++i)
                id.Low = (id.Low << 8) | bytes[i];

            return id;
        }

    }

    SceneObjectHandle Scene::CreateObject()
    {
        constexpr auto invalidIndex = SceneObjectHandle::InvalidIndex;

        // 没有可复用槽位时，InvalidIndex 不能成为有效对象下标。
        if (m_FreeHead == invalidIndex && m_ObjectSlots.getSize() >= invalidIndex)
            return {};

        SceneObjectID id{};
        while (true)
        {
            id = GenerateSceneObjectID();
            if (!id.isValid())
                continue;

            bool duplicate = false;
            for (const ObjectSlot &slot: m_ObjectSlots)
            {
                if (slot.Alive && slot.ID == id)
                {
                    duplicate = true;
                    break;
                }
            }
            if (!duplicate)
                break;
        }

        std::uint32_t index;
        if (m_FreeHead != invalidIndex)
        {
            index = m_FreeHead;
            const bool validFreeSlot =
                    index < m_ObjectSlots.getSize() &&
                    !m_ObjectSlots[index].Alive &&
                    m_ObjectSlots[index].Generation != 0;

            LM_CORE_ASSERT(validFreeSlot, "Scene object free list is corrupted");
            if (!validFreeSlot)
                return {};

            m_FreeHead = m_ObjectSlots[index].NextFree;
        }
        else
        {
            index = m_ObjectSlots.getSize();
            m_ObjectSlots.emplace_back();
        }

        auto &[ID, Transform, Generation, NextFree, Alive] = m_ObjectSlots[index];
        ID = id;
        Transform = glm::mat4{1.0f};
        NextFree = invalidIndex;
        Alive = true;

        return {
            .Index = index,
            .Generation = Generation
        };
    }

    bool Scene::DestroyObject(const SceneObjectHandle handle)
    {
        if (!handle.IsValid() || handle.Index >= m_ObjectSlots.getSize())
            return false;

        ObjectSlot &slot = m_ObjectSlots[handle.Index];
        if (!slot.Alive || slot.Generation != handle.Generation)
            return false;

        // 先使对象及其持久身份失效，再把槽位接入空闲链表。
        slot.ID = {};
        slot.Transform = glm::mat4{1.0f};
        slot.Alive = false;

        constexpr auto invalidIndex = SceneObjectHandle::InvalidIndex;
        constexpr static auto maxGeneration = std::numeric_limits<std::uint32_t>::max();

        // 不允许 Generation 回绕：否则一个极端旧的 Handle 可能重新有效。
        // 该槽位退休，不再加入空闲链。
        if (slot.Generation == maxGeneration)
        {
            slot.NextFree = invalidIndex;
            return true;
        }

        ++slot.Generation;
        slot.NextFree = m_FreeHead;
        m_FreeHead = handle.Index;
        return true;
    }

    bool Scene::IsObjectAlive(const SceneObjectHandle handle) const noexcept
    {
        if (!handle.IsValid() || handle.Index >= m_ObjectSlots.getSize())
            return false;

        const ObjectSlot &slot = m_ObjectSlots[handle.Index];
        return slot.Alive && slot.Generation == handle.Generation;
    }

    bool Scene::TryGetObjectID(
        const SceneObjectHandle handle,
        SceneObjectID &outID
    ) const noexcept
    {
        if (!IsObjectAlive(handle))
            return false;

        outID = m_ObjectSlots[handle.Index].ID;
        return true;
    }

    bool Scene::TryFindObject(
        const SceneObjectID id,
        SceneObjectHandle &outHandle
    ) const noexcept
    {
        if (!id.isValid())
            return false;

        for (std::uint32_t index = 0; index < m_ObjectSlots.getSize(); ++index)
        {
            const ObjectSlot &slot = m_ObjectSlots[index];
            if (!slot.Alive || slot.ID != id)
                continue;

            outHandle = {
                .Index = index,
                .Generation = slot.Generation
            };
            return true;
        }

        return false;
    }

    bool Scene::SetObjectTransform(
        const SceneObjectHandle handle,
        const glm::mat4 &transform
    )
    {
        if (!IsObjectAlive(handle))
            return false;

        m_ObjectSlots[handle.Index].Transform = transform;
        return true;
    }

    bool Scene::TryGetObjectTransform(
        const SceneObjectHandle handle,
        glm::mat4 &outTransform
    ) const noexcept
    {
        if (!IsObjectAlive(handle))
            return false;

        outTransform = m_ObjectSlots[handle.Index].Transform;
        return true;
    }

    void Scene::ForEachObject(
        const std::function<void(const SceneObjectHandle &, const SceneObjectID &)> &callback) const
    {
        for (std::uint32_t index = 0; index < m_ObjectSlots.getSize(); ++index)
        {
            const ObjectSlot& slot = m_ObjectSlots[index];
            if (!slot.Alive)
                continue;

            callback(SceneObjectHandle{.Index = index, .Generation = slot.Generation},slot.ID);
        }
    }

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
            m_RenderObjects.getSize() < SceneRenderObjectHandle::InvalidIndex,
            "Scene contains too many render objects"
        );

        if (m_RenderObjects.getSize() >= SceneRenderObjectHandle::InvalidIndex)
            return {};

        const uint32_t objectIdx = static_cast<uint32_t>(m_RenderObjects.getSize());
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
        const float directionalLengthSquared =
                glm::dot(light.Direction, light.Direction);

        const bool directionIsValid = directionalLengthSquared > 0.0f;
        const bool intensityIsValid = light.Intensity >= 0.0f;

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

    void Scene::SetAmbientLight(const AmbientLight &light)
    {
        const bool intensityIsValid = light.Intensity >= 0.0f;

        LM_CORE_ASSERT(
            intensityIsValid,
            "AmbientLight intensity cannot be negative"
        );

        if (!intensityIsValid)
            return;

        m_AmbientLight = light;
    }

    const AmbientLight &Scene::GetAmbientLight() const noexcept
    {
        return m_AmbientLight;
    }

    ScenePointLightHandle Scene::AddPointLight(const PointLight &light)
    {
        const bool intensityIsValid = light.Intensity >= 0.0f;

        LM_CORE_ASSERT(
            intensityIsValid,
            "PointLight intensity cannot be negative"
        );

        if (!intensityIsValid)
            return {};

        /*
         * InvalidIndex 被保留为“无效句柄”，
         * 所以不能让有效元素使用这个下标。
         */
        LM_CORE_ASSERT(
            m_PointLights.getSize() < ScenePointLightHandle::InvalidIndex,
            "Scene contains too many point lights"
        );

        if (m_PointLights.getSize() >= ScenePointLightHandle::InvalidIndex)
            return {};

        ScenePointLightHandle handle;
        handle.Index = m_PointLights.getSize();
        m_PointLights.push_back(light);
        return handle;
    }

    bool Scene::SetPointLight(
        const ScenePointLightHandle handle,
        const PointLight &light
    )
    {
        const bool handleIsValid =
                handle.IsValid() && handle.Index < m_PointLights.getSize();

        LM_CORE_ASSERT(
            handleIsValid,
            "Scene cannot update invalid point light handle '{}'",
            handle.Index
        );

        if (!handleIsValid)
            return false;

        const bool intensityIsValid = light.Intensity >= 0.0f;

        LM_CORE_ASSERT(
            intensityIsValid,
            "PointLight intensity cannot be negative"
        );

        // Release 构建仍然需要阻止无效光源数据写入 Scene。
        if (!intensityIsValid)
            return false;

        m_PointLights[handle.Index] = light;
        return true;
    }

    bool Scene::TryGetPointLight(
        const ScenePointLightHandle handle,
        PointLight &outLight
    ) const noexcept
    {
        const bool handleIsValid =
                handle.IsValid() &&
                handle.Index < m_PointLights.getSize();

        /*
         * 函数名使用 Try，表示查询失败属于正常结果，
         * 因此这里不触发断言，只返回 false。
         */
        if (!handleIsValid)
            return false;

        /*
         * 将 Scene 中的光源数据复制到调用者自己的对象中。
         * 调用者之后修改 outLight，不会绕过 SetPointLight 修改 Scene。
         */
        outLight = m_PointLights[handle.Index];
        return true;
    }

    const LArray<PointLight> &Scene::GetPointLights() const noexcept
    {
        return m_PointLights;
    }

    const DirectionalLight &Scene::GetDirectionalLight() const noexcept
    {
        return m_DirectionalLight;
    }

    bool Scene::SetRenderObjectTransform(
        const SceneRenderObjectHandle handle,
        const glm::mat4 &transform
    )
    {
        const bool handleIsValid =
                handle.IsValid() && handle.Index < m_RenderObjects.getSize();

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

    const LArray<SceneRenderObject> &Scene::GetRenderObjects() const noexcept
    {
        return m_RenderObjects;
    }
}
