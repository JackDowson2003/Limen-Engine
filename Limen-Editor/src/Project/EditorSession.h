//
// Created by chenlong on 2026/10/21.
//

#pragma once
#include <utility>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Project/ProjectConfiguration.h"
#include "Limen/Scene/Scene.h"

namespace LimenEditor
{
    class EditorSession final
    {
    public:
        explicit EditorSession(Limen::ProjectConfiguration configuration,Limen::Scope<Limen::AssetRegistry> registry)
            : m_Configuration(std::move(configuration)),m_AssetRegistry(std::move(registry)),
m_Scene(Limen::CreateScope<Limen::Scene>())
        {}

        [[nodiscard]]
        const Limen::ProjectConfiguration& GetConfiguration() const noexcept
        {
            return m_Configuration;
        }

        [[nodiscard]]
        const Limen::AssetRegistry& GetAssetRegistry() const noexcept
        {
            return *m_AssetRegistry;
        }

        // 创建场景普通对象并返回其稳定的ID，失败的时候返回 High/Low 均为0的无效ID
        // 第二部会把 " 创建即选中 " 连接到这里
        Limen::SceneObjectID CreateObject()
        {
            const Limen::SceneObjectHandle handle = m_Scene->CreateObject();

            Limen::SceneObjectID id{};
            if (!m_Scene->TryGetObjectID(handle,id))
                return {};
            // 创建即选中：新对象自动成为当前选择。
            m_SelectedObjectID = id;
            MarkDirty();
            return id;
        }

        // 创建一个带渲染组件的对象；失败返回无效 ID，成功则选中并置脏。
        [[nodiscard]]
        Limen::SceneObjectID CreateRenderableObject(
            Limen::Ref<Limen::Mesh> mesh,
            Limen::Ref<Limen::Material> material)
        {
            if (!mesh || !material)
                return {};

            const Limen::SceneObjectHandle handle =
                m_Scene->AddRenderableObject(
                    {
                        .MeshResource = std::move(mesh),
                        .MaterialResource = std::move(material),
                        .Transform = glm::mat4{1.0f}
                    });
            Limen::SceneObjectID id{};
            if (!m_Scene->TryGetObjectID(handle, id))
                return {};

            m_SelectedObjectID = id;
            MarkDirty();
            return id;
        }

        // 面板与视口只借用活动场景；一切修改必须经过会话的编辑方法。
        [[nodiscard]]
        const Limen::Scene& GetScene() const noexcept
        {
            return *m_Scene;
        }

        // 选择只存稳定ID，失效或删除时由会话负责清除
        bool SelectObject(const Limen::SceneObjectID id)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id,handle))
                return false;
            m_SelectedObjectID = id;
            return true;
        }

        void ClearSelection() noexcept
        {
            m_SelectedObjectID = {};
        }

        [[nodiscard]]
        Limen::SceneObjectID GetSelectedObjectID() const noexcept
        {
            return m_SelectedObjectID;
        }

        [[nodiscard]]
        bool IsObjectSelected(const Limen::SceneObjectID id) const noexcept
        {
            return m_SelectedObjectID.isValid() && m_SelectedObjectID == id;
        }

        // 删除指定对象：成功则置脏；若删除的是当前选择，同时清除选择
        bool DeleteObject(const Limen::SceneObjectID id)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id,handle))
                return false;
            if (!m_Scene->DestroyObject(handle))
                return false;

            if (m_SelectedObjectID == id)
                m_SelectedObjectID = {};
            MarkDirty();
            return true;
        }

        // 修改当前选择对象的变换；无选择或修改失败时返回 false。
        bool SetSelectedObjectTransform(const glm::mat4& transform)
        {
            if (!m_SelectedObjectID.isValid())
                return false;
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(m_SelectedObjectID, handle))
                return false;
            if (!m_Scene->SetObjectTransform(handle, transform))
                return false;

            MarkDirty();
            return true;
        }

        // 读取当前对象的transform 失败时返回false 且不修改out transform
        [[nodiscard]]
        bool TryGetSelectedObjectTransform(glm::mat4& outTransform) const noexcept
        {
            if (!m_SelectedObjectID.isValid())
                return false;
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(m_SelectedObjectID, handle))
                return false;
            return m_Scene->TryGetObjectTransform(handle,outTransform);
        }

        [[nodiscard]]
        bool IsDirty() const noexcept
        {
            return m_IsDirty;
        }

        // 保存成功后调用；当前保存尚未接入，先留给测试与将来的保存流程。
        void ClearDirty() noexcept
        {
            m_IsDirty = false;
        }

    private:
        // 会话拥有已校验的配置；不借用 CreateApplication 中的局部变量。
        Limen::ProjectConfiguration m_Configuration;
        Limen::Scope<Limen::AssetRegistry> m_AssetRegistry;

        // 会话独占的活动场景，面板只通过GetScene() 借用，不拥有，不释放
        Limen::Scope<Limen::Scene> m_Scene;
        // 当前选中对象的稳定 ID；无效 ID 表示没有选择。
        Limen::SceneObjectID m_SelectedObjectID{};

        // session是否有未保存的编辑，只有会话自己的编辑方法才能置为脏
        bool m_IsDirty = false;

    private:
        void MarkDirty() noexcept
        {
            m_IsDirty = true;
        }
    };
}