//
// Created by chenlong on 2026/9/7.
//

#include <glm/geometric.hpp>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <random>
#include <utility>

#include "Limen/Scene/Scene.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/Mesh.h"
#include "Limen/Scene/SceneIdentity.h"

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

        const auto index = AcquireObjectSlot();
        if (index == SceneObjectHandle::InvalidIndex)
            return {};
        auto &[
            ID,
            AuthoringTransform,
            Generation,
            NextFree,
            Alive,
            RenderComponents,
            Components,
            Label
        ] = m_ObjectSlots[index];
        ID = id;
        // 新对象的 Authoring TRS 被重置为单位变换。
        AuthoringTransform = SceneTransform{};
        NextFree = invalidIndex;
        Alive = true;
        RenderComponents.clear();
        // 新对象和复用槽位都不能残留上一代 Authoring Component。
        Components.clear();
        // 新对象默认无 Label；复用槽位时也必须清除旧名称。
        // clear 保留已分配容量，避免频繁创建和销毁时重复分配字符串。
        Label.clear();

        return {
            .Index = index,
            .Generation = Generation
        };
    }

    SceneObjectHandle Scene::Restore(const SceneObjectAuthoringData &data)
    {
        if (!data.ID.isValid() || !data.Transform.IsFinite())
            return {};

        for (std::uint32_t i = 0; i < data.Components.getSize(); ++i)
        {
            const auto &component = data.Components[i];
            if (!IsSceneObjectComponentValid(component))
                return {};

            // 组件id必须唯一
            for (std::uint32_t j = 0; j < i; j++)
                if (data.Components[j].ID == component.ID)
                    return {};
        }

        SceneObjectHandle existing{};
        // 查找是否已经存在
        if (TryFindObject(data.ID, existing))
            return {};

        // 拷贝的内存可能分配，所以先准备好数据再修改空闲链
        LString label(data.Label);
        LArray<SceneObjectComponent> components(data.Components);

        // 取到空闲槽位
        const auto index = AcquireObjectSlot();
        if (index == SceneObjectHandle::InvalidIndex)
            return {};

        ObjectSlot &slot = m_ObjectSlots[index];

        slot.ID = data.ID;
        slot.Label = std::move(label);
        slot.Components = std::move(components);
        slot.AuthoringTransform = data.Transform;
        slot.RenderComponents.clear();
        slot.Alive = true;

        return {.Index = index, .Generation = slot.Generation};
    }

    std::uint32_t Scene::AcquireObjectSlot()
    {
        std::uint32_t index;
        if (m_FreeHead != SceneObjectHandle::InvalidIndex)
        {
            index = m_FreeHead;
            const bool validFreeSlot =
                    index < m_ObjectSlots.getSize() &&
                    !m_ObjectSlots[index].Alive &&
                    m_ObjectSlots[index].Generation != 0;

            LM_CORE_ASSERT(validFreeSlot, "Scene object free list is corrupted");
            if (!validFreeSlot)
                return SceneObjectHandle::InvalidIndex;

            ObjectSlot &slot = m_ObjectSlots[index];
            m_FreeHead = slot.NextFree;
            slot.NextFree = SceneObjectHandle::InvalidIndex;
        } else
        {
            index = m_ObjectSlots.getSize();
            if (index >= SceneObjectHandle::InvalidIndex)
                return SceneObjectHandle::InvalidIndex;
            m_ObjectSlots.emplace_back();
        }
        return index;
    }

    SceneObjectHandle Scene::AddRenderableObject(const SceneRenderObject &renderObject,
                                                 const SceneTransform &objectTransform)
    {
        if (!renderObject.MeshResource || !renderObject.MaterialResource || !objectTransform.IsFinite())
            return {};

        LArray<SceneRenderObject> preparedParts{renderObject};

        const SceneObjectHandle &handle = CreateObject();
        if (!handle.IsValid() || handle.Index >= m_ObjectSlots.getSize())
            return {};

        ObjectSlot &slot = m_ObjectSlots[handle.Index];
        slot.AuthoringTransform = objectTransform;
        slot.RenderComponents = std::move(preparedParts);
        return handle;
    }

    SceneObjectHandle Scene::AddRenderableObjectGroup(const LArray<SceneRenderObject> &parts,
                                                      const SceneTransform &objectTransform)
    {
        if (parts.empty() || !objectTransform.IsFinite())
            return {};

        for (const SceneRenderObject & part : parts)
        {
            if (!part.MeshResource || !part.MaterialResource)
                return {};
        }

        // 拷贝可能分配内存，先准备部件，再创建场景对象。
        LArray preparedParts(parts);

        const SceneObjectHandle & handle = CreateObject();
        if (!handle.IsValid())
            return {};

        ObjectSlot &slot = m_ObjectSlots[handle.Index];
        slot.AuthoringTransform = objectTransform;
        slot.RenderComponents = std::move(preparedParts);
        return handle;
    }

    void Scene::ForEachRenderableObject(
        const std::function<void
            (const SceneObjectHandle &, const SceneObjectID &, const Ref<Mesh> &, const Ref<Material> &,
             const glm::mat4 &)> &callback) const
    {
        for (uint32_t index = 0; index < m_ObjectSlots.getSize(); ++index)
        {
            const ObjectSlot &slot = m_ObjectSlots[index];
            if (!slot.Alive || slot.RenderComponents.getSize() == 0)
                continue;

            const glm::mat4 objectToWorld = slot.AuthoringTransform.ToMatrix();
            for (const auto &[MeshResource, MaterialResource, Transform]: slot.RenderComponents)
            {
                callback(
                    SceneObjectHandle{
                        .Index = index,
                        .Generation = slot.Generation
                    },
                    slot.ID,
                    MeshResource,
                    MaterialResource,
                    objectToWorld * Transform
                );
            }
        }
    }

    bool Scene::PickObject(
        const glm::vec3 &rayOrigin,
        const glm::vec3 &rayDirection,
        SceneObjectID &outID) const noexcept
    {
        const float directionLengthSquared =
                glm::dot(rayDirection, rayDirection);
        if (directionLengthSquared <= 0.0f)
            return false;

        const glm::vec3 direction =
                rayDirection / std::sqrt(directionLengthSquared);

        float closestDistance = std::numeric_limits<float>::max();
        SceneObjectID closestID{};
        bool found = false;

        for (uint32_t index = 0; index < m_ObjectSlots.getSize(); ++index)
        {
            const ObjectSlot &slot = m_ObjectSlots[index];
            if (!slot.Alive || slot.RenderComponents.getSize() == 0)
                continue;

            const glm::mat4 objectTransform = slot.AuthoringTransform.ToMatrix();
            for (const SceneRenderObject &component: slot.RenderComponents)
            {
                if (!component.MeshResource)
                    continue;

                const AxisAlignedBoundingBox worldBounds =
                        component.MeshResource->GetLocalBounds().Transformed(
                            objectTransform * component.Transform);

                float distance = 0.0f;
                if (!IntersectRayAABB(
                    worldBounds,
                    rayOrigin,
                    direction,
                    distance))
                    continue;

                if (distance < closestDistance)
                {
                    closestDistance = distance;
                    closestID = slot.ID;
                    found = true;
                }
            }
        }

        if (found)
            outID = closestID;
        return found;
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
        // 销毁后恢复默认 TRS，防止槽位复用残留上一代变换。
        slot.AuthoringTransform = SceneTransform{};
        slot.Alive = false;
        slot.RenderComponents.clear();
        slot.Components.clear();
        slot.Label.clear();

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

    bool Scene::SetObjectTransform(const SceneObjectHandle handle, const SceneTransform &transform)
    {
        if (!IsObjectAlive(handle) || !transform.IsFinite())
            return false;
        ObjectSlot &slot = m_ObjectSlots[handle.Index];
        slot.AuthoringTransform = transform;
        return true;
    }

    bool Scene::TryGetObjectTransform(const SceneObjectHandle handle, SceneTransform &outTransform) const noexcept
    {
        if (!IsObjectAlive(handle))
            return false;
        outTransform = m_ObjectSlots[handle.Index].AuthoringTransform;
        return true;
    }

    bool Scene::SetObjectAuthoringTransform(const SceneObjectHandle handle, const SceneTransform &transform)
    {
        if (!IsObjectAlive(handle) || !transform.IsFinite())
            return false;
        ObjectSlot &slot = m_ObjectSlots[handle.Index];
        slot.AuthoringTransform = transform;
        return true;
    }

    bool Scene::TryGetObjectAuthoringTransform(const SceneObjectHandle handle,
                                               SceneTransform &outTransform) const noexcept
    {
        if (!IsObjectAlive(handle))
            return false;
        outTransform = m_ObjectSlots[handle.Index].AuthoringTransform;
        return true;
    }

    bool Scene::TryGetObjectTransform(
        const SceneObjectHandle handle,
        glm::mat4 &outTransform
    ) const noexcept
    {
        if (!IsObjectAlive(handle))
            return false;

        outTransform = m_ObjectSlots[handle.Index].AuthoringTransform.ToMatrix();
        return true;
    }

    bool Scene::SetObjectLabel(const SceneObjectHandle handle, std::string_view label)
    {
        if (!IsObjectAlive(handle))
            return false;

        m_ObjectSlots[handle.Index].Label = LString(label);
        return true;
    }

    bool Scene::TryGetObjectLabel(const SceneObjectHandle handle, LString &outLabel) const
    {
        if (!IsObjectAlive(handle))
            return false;

        // 返回副本，避免调用方引用槽位内部字符串。
        outLabel = m_ObjectSlots[handle.Index].Label;
        return true;
    }

    void Scene::ForEachObjectAuthoring(const std::function<void(const SceneObjectAuthoringData &)> &callback) const
    {
        for (std::uint32_t index = 0; index < m_ObjectSlots.getSize(); ++index)
        {
            const ObjectSlot &slot = m_ObjectSlots[index];
            if (!slot.Alive)
                continue;

            // 每次构造快照，避免取得Scene内部引用
            const SceneObjectAuthoringData snapshot{
                .ID = slot.ID,
                .Label = slot.Label,
                .Transform = slot.AuthoringTransform,
                .Components = slot.Components
            };
            callback(snapshot);
        }
    }

    void Scene::ForEachObject(
        const std::function<void(const SceneObjectHandle &, const SceneObjectID &)> &callback) const
    {
        for (std::uint32_t index = 0; index < m_ObjectSlots.getSize(); ++index)
        {
            const ObjectSlot &slot = m_ObjectSlots[index];
            if (!slot.Alive)
                continue;

            callback(SceneObjectHandle{.Index = index, .Generation = slot.Generation}, slot.ID);
        }
    }

    SceneComponentID Scene::AddObjectComponent(const SceneObjectHandle handle, SceneObjectComponent component)
    {
        if (!IsObjectAlive(handle))
            return {};

        // 调用方可预分配 ID 供加载恢复使用；普通创建时自动生成。
        if (!component.ID.isValid())
            component.ID = GenerateSceneComponentID();

        if (!IsSceneObjectComponentValid(component))
            return {};

        LArray<SceneObjectComponent> &components = m_ObjectSlots[handle.Index].Components;

        for (const auto &existing: components)
        {
            if (existing.ID == component.ID)
                return {};
        }
        const SceneComponentID id = component.ID;
        components.push_back(std::move(component));
        return id;
    }

    void Scene::ForEachObjectComponent(SceneObjectHandle handle,
                                       const std::function<void(const SceneObjectComponent &)> &callback) const
    {
        if (!IsObjectAlive(handle))
            return;
        for (const SceneObjectComponent &component: m_ObjectSlots[handle.Index].Components)
        {
            callback(component);
        }
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
}
