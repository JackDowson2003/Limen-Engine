//
// Created by chenlong on 2026/10/21.
//

#pragma once
#include <functional>
#include <string_view>
#include <utility>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Core/Log.h"
#include "Limen/Project/ProjectConfiguration.h"
#include "Limen/Scene/Scene.h"

namespace LimenEditor
{
    class EditorSession final
    {
    public:
        explicit EditorSession(Limen::ProjectConfiguration configuration, Limen::Scope<Limen::AssetRegistry> registry)
            : m_Configuration(std::move(configuration)), m_AssetRegistry(std::move(registry)),
              m_Scene(Limen::CreateScope<Limen::Scene>())
        {
        }

        [[nodiscard]]
        const Limen::ProjectConfiguration &GetConfiguration() const noexcept
        {
            return m_Configuration;
        }

        [[nodiscard]]
        const Limen::AssetRegistry &GetAssetRegistry() const noexcept
        {
            return *m_AssetRegistry;
        }

        // 资产选择独立于场景对象选择；只接受 Registry 中存在的资产。
        [[nodiscard]]
        bool SelectAsset(const Limen::AssetID id)
        {
            Limen::AssetRecord record;
            if (!m_AssetRegistry->TryFind(id, record))
                return false;
            m_SelectedAssetID = id;
            return true;
        }

        void ClearAssetSelection() noexcept
        {
            m_SelectedAssetID = {};
        }

        [[nodiscard]]
        Limen::AssetID GetSelectedAssetID() const noexcept
        {
            return m_SelectedAssetID;
        }

        [[nodiscard]]
        bool IsAssetSelected(const Limen::AssetID id) const noexcept
        {
            return m_SelectedAssetID.isValid() && m_SelectedAssetID == id;
        }

        // 创建场景普通对象并返回其稳定的ID，失败的时候返回 High/Low 均为0的无效ID
        // 第二部会把 " 创建即选中 " 连接到这里
        Limen::SceneObjectID CreateObject()
        {
            const Limen::SceneObjectHandle handle = m_Scene->CreateObject();

            Limen::SceneObjectID id{};
            if (!m_Scene->TryGetObjectID(handle, id))
                return {};
            SelectSingleObject(id);
            MarkDirty();
            return id;
        }

        // 创建一个带渲染组件的对象；失败返回无效 ID，成功则选中并置脏。
        [[nodiscard]]
        Limen::SceneObjectID CreateRenderableObject(
            Limen::Ref<Limen::Mesh> mesh,
            Limen::Ref<Limen::Material> material,
            const Limen::SceneTransform &transform = {},
            const std::string_view label = {})
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
            if (!m_Scene->SetObjectTransform(handle, transform))
            {
                static_cast<void>(m_Scene->DestroyObject(handle));
                return {};
            }
            if (!m_Scene->SetObjectLabel(handle, label))
            {
                static_cast<void>(m_Scene->DestroyObject(handle));
                return {};
            }
            Limen::SceneObjectID id{};
            if (!m_Scene->TryGetObjectID(handle, id))
                return {};
            SelectSingleObject(id);
            MarkDirty();
            return id;
        }

        // 一个多 Part 模型对应一个 SceneObject，内部挂多个渲染组件。
        [[nodiscard]]
        Limen::SceneObjectID CreateRenderableObjectGroup(
            const Limen::LArray<Limen::SceneRenderObject> &parts,
            const Limen::SceneTransform &transform = {},
            const std::string_view label = {})
        {
            if (parts.getSize() == 0)
                return {};

            const Limen::SceneObjectHandle handle =
                m_Scene->AddRenderableObjectGroup(parts);
            if (!m_Scene->SetObjectTransform(handle, transform))
            {
                static_cast<void>(m_Scene->DestroyObject(handle));
                return {};
            }
            if (!m_Scene->SetObjectLabel(handle, label))
            {
                static_cast<void>(m_Scene->DestroyObject(handle));
                return {};
            }

            Limen::SceneObjectID id{};
            if (!m_Scene->TryGetObjectID(handle, id))
                return {};

            SelectSingleObject(id);
            MarkDirty();
            return id;
        }

        // 面板与视口只借用活动场景；一切修改必须经过会话的编辑方法。
        [[nodiscard]]
        const Limen::Scene &GetScene() const noexcept
        {
            return *m_Scene;
        }

        // 普通点击：以单个对象替换当前选择。
        bool SelectObject(const Limen::SceneObjectID id)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;
            SelectSingleObject(id);
            return true;
        }

        // Cmd/Ctrl+点击：命中则移出选择，未命中则追加。
        bool ToggleObjectSelection(const Limen::SceneObjectID id)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;

            for (uint32_t i = 0; i < m_SelectedObjectIDs.getSize(); ++i)
            {
                if (m_SelectedObjectIDs[i] == id)
                {
                    m_SelectedObjectIDs[i] =
                            m_SelectedObjectIDs[m_SelectedObjectIDs.getSize() - 1];
                    m_SelectedObjectIDs.pop_back();
                    return true;
                }
            }
            m_SelectedObjectIDs.push_back(id);
            return true;
        }

        // Shift 范围选择：用经过校验的去重集合替换当前选择。
        bool SelectObjects(const Limen::LArray<Limen::SceneObjectID> &ids)
        {
            Limen::LArray<Limen::SceneObjectID> validated;
            for (const Limen::SceneObjectID id: ids)
            {
                Limen::SceneObjectHandle handle{};
                if (!m_Scene->TryFindObject(id, handle))
                    continue;

                bool duplicate = false;
                for (uint32_t i = 0; i < validated.getSize(); ++i)
                {
                    if (validated[i] == id)
                    {
                        duplicate = true;
                        break;
                    }
                }
                if (!duplicate)
                    validated.push_back(id);
            }

            const bool hasAny = validated.getSize() > 0;
            m_SelectedObjectIDs = validated;
            return hasAny;
        }

        void ClearSelection() noexcept
        {
            m_SelectedObjectIDs.clear();
        }

        // 恰好选中一个时返回该 ID；零个或多个时返回无效 ID。
        [[nodiscard]]
        Limen::SceneObjectID GetSelectedObjectID() const noexcept
        {
            return m_SelectedObjectIDs.getSize() == 1
                       ? m_SelectedObjectIDs[0]
                       : Limen::SceneObjectID{};
        }

        [[nodiscard]]
        const Limen::LArray<Limen::SceneObjectID> &
        GetSelectedObjectIDs() const noexcept
        {
            return m_SelectedObjectIDs;
        }

        [[nodiscard]]
        uint32_t GetSelectedObjectCount() const noexcept
        {
            return m_SelectedObjectIDs.getSize();
        }

        [[nodiscard]]
        bool IsObjectSelected(const Limen::SceneObjectID id) const noexcept
        {
            if (!id.isValid())
                return false;

            for (uint32_t i = 0; i < m_SelectedObjectIDs.getSize(); ++i)
            {
                if (m_SelectedObjectIDs[i] == id)
                    return true;
            }
            return false;
        }

        // 删除指定对象：成功则置脏，并从选择集合移除。
        bool DeleteObject(const Limen::SceneObjectID id)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;
            if (!m_Scene->DestroyObject(handle))
                return false;

            RemoveFromSelection(id);
            MarkDirty();
            return true;
        }

        // 删除全部选中对象；只要成功删除一个，就清空选择并置脏
        bool DeleteSelectedObjects()
        {
            if (m_SelectedObjectIDs.empty())
                return false;

            // 先复制一份ID 删除过程中会修改选择集合
            const Limen::LArray<Limen::SceneObjectID> selected = m_SelectedObjectIDs;

            bool deleteAny = false;
            for (const Limen::SceneObjectID id : selected)
            {
                Limen::SceneObjectHandle handle{};
                if (!m_Scene->TryFindObject(id,handle))
                    continue;
                if (!m_Scene->DestroyObject(handle))
                    continue;
                deleteAny = true;
            }
            m_SelectedObjectIDs.clear();
            if (deleteAny)
                MarkDirty();
            return deleteAny;
        }

        // 多选暂不做批量变换：只有恰好一个对象时生效。
        // Authoring 编辑入口：只写入 TRS，由 Scene 同步运行期矩阵缓存。
        bool SetSelectedObjectTransform(
            const Limen::SceneTransform &transform)
        {
            if (m_SelectedObjectIDs.getSize() != 1)
                return false;

            const Limen::SceneObjectID selected = m_SelectedObjectIDs[0];
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(selected, handle))
                return false;
            if (!m_Scene->SetObjectTransform(handle, transform))
                return false;

            MarkDirty();
            return true;
        }

        [[nodiscard]]
        bool TryGetSelectedObjectTransform(glm::mat4 &outTransform) const noexcept
        {
            if (m_SelectedObjectIDs.getSize() != 1)
                return false;

            const Limen::SceneObjectID selected = m_SelectedObjectIDs[0];
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(selected, handle))
                return false;
            return m_Scene->TryGetObjectTransform(handle, outTransform);
        }

        [[nodiscard]]
        bool TryGetSelectedObjectTransform(
            Limen::SceneTransform &outTransform) const noexcept
        {
            if (m_SelectedObjectIDs.getSize() != 1)
                return false;

            return TryGetObjectTransform(
                m_SelectedObjectIDs[0],
                outTransform);
        }

        // 按稳定 ID 读取任意对象 Transform；失败时不修改 outTransform。
        [[nodiscard]]
        bool TryGetObjectTransform(
            const Limen::SceneObjectID id,
            glm::mat4 &outTransform) const noexcept
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;
            return m_Scene->TryGetObjectTransform(handle, outTransform);
        }

        [[nodiscard]]
        bool TryGetObjectTransform(
            const Limen::SceneObjectID id,
            Limen::SceneTransform &outTransform) const noexcept
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;
            return m_Scene->TryGetObjectTransform(handle, outTransform);
        }

        // Label 是可编辑的显示名称，修改成功会使场景进入脏状态。
        bool SetObjectLabel(
            const Limen::SceneObjectID id,
            const std::string_view label)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;
            if (!m_Scene->SetObjectLabel(handle, label))
                return false;

            MarkDirty();
            return true;
        }

        // 按稳定 ID 读取显示名称；失败时不修改 outLabel。
        [[nodiscard]]
        bool TryGetObjectLabel(
            const Limen::SceneObjectID id,
            Limen::LString &outLabel) const
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return false;
            return m_Scene->TryGetObjectLabel(handle, outLabel);
        }

        // 给现有对象添加 Authoring Component；成功会置脏。
        [[nodiscard]]
        Limen::SceneComponentID AddObjectComponent(
            const Limen::SceneObjectID id,
            Limen::SceneObjectComponent component)
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return {};

            const Limen::SceneComponentID componentID =
                m_Scene->AddObjectComponent(handle, std::move(component));
            if (componentID.isValid())
                MarkDirty();
            return componentID;
        }

        // 面板通过稳定对象 ID 只读遍历组件，不暴露运行期 SceneObjectHandle。
        void ForEachObjectComponent(
            const Limen::SceneObjectID id,
            const std::function<void(
                const Limen::SceneObjectComponent &)> &callback) const
        {
            Limen::SceneObjectHandle handle{};
            if (!m_Scene->TryFindObject(id, handle))
                return;
            m_Scene->ForEachObjectComponent(handle, callback);
        }

        [[nodiscard]]
        bool TryGetSelectedObjectLabel(Limen::LString &outLabel) const
        {
            return m_SelectedObjectIDs.getSize() == 1 &&
                   TryGetObjectLabel(m_SelectedObjectIDs[0], outLabel);
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
        // 当前选中的场景对象集合，顺序不参与语义。
        Limen::LArray<Limen::SceneObjectID> m_SelectedObjectIDs;
        // Content Browser 当前选中的资产；无效 ID 表示未选择。
        Limen::AssetID m_SelectedAssetID{};

        // session是否有未保存的编辑，只有会话自己的编辑方法才能置为脏
        bool m_IsDirty = false;

    private:
        void MarkDirty() noexcept
        {
            m_IsDirty = true;
        }

        void SelectSingleObject(const Limen::SceneObjectID id)
        {
            m_SelectedObjectIDs.clear();
            m_SelectedObjectIDs.push_back(id);
        }

        void RemoveFromSelection(const Limen::SceneObjectID id) noexcept
        {
            for (uint32_t i = 0; i < m_SelectedObjectIDs.getSize();)
            {
                if (m_SelectedObjectIDs[i] != id)
                {
                    ++i;
                    continue;
                }
                m_SelectedObjectIDs[i] = m_SelectedObjectIDs[m_SelectedObjectIDs.getSize() - 1];
                m_SelectedObjectIDs.pop_back();
            }
        }
    };
}
