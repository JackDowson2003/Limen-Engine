#include "UI/EditorUILayer.h"

#include "Render/EditorSceneRenderParts.h"
#include "UI/EditorSceneDialogs.h"

namespace LimenEditor
{
    void EditorUILayer::OpenSceneFromPath(const std::filesystem::path &path)
    {
        if (!m_Session)
            return;

        const auto &registry = m_Session->GetAssetRegistry();
        const auto resolve = [&](const Limen::SceneObjectComponent &component,
                                 Limen::LArray<Limen::SceneRenderObject> &parts,
                                 Limen::SceneFileError &error)
        {
            return TryResolveEditorSceneRenderParts(registry, m_SceneContent, component, parts, error);
        };

        Limen::SceneFileError error;
        if (!m_Session->TryOpenScene(path, resolve, error))
        {
            LM_ERROR("打开场景失败: file='{}', key='{}', reason='{}'",
                     error.File.string(), error.Key, error.Message);
            ShowStatus("Cannot open scene. See log for details.", EditorStatusKind::Error);
            return;
        }

        m_Outliner.SetSelectionAnchor({});
        LM_INFO("已打开场景: file='{}'", m_Session->GetScenePath().string());
        const auto sceneLabel = m_Session->GetActiveSceneLabel();
        LoadWorkspaceFont(sceneLabel);
        ShowStatus("Opened scene " + sceneLabel, EditorStatusKind::Info);
    }

    bool EditorUILayer::HandleSceneShortcut(const Limen::KeyPressedEvent &key)
    {
        using Limen::Input;
        using Limen::KeyCode;
        const bool command = Input::IsKeyPressed(KeyCode::LeftSuper) || Input::IsKeyPressed(KeyCode::RightSuper) ||
                             Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
        if (!command)
            return false;

        SceneToolbarAction action;
        switch (key.GetKeyCode())
        {
            case KeyCode::N: action = SceneToolbarAction::New; break;
            case KeyCode::O: action = SceneToolbarAction::Open; break;
            case KeyCode::S:
            {
                const bool shift = Input::IsKeyPressed(KeyCode::LeftShift) || Input::IsKeyPressed(KeyCode::RightShift);
                action = shift ? SceneToolbarAction::SaveAs : SceneToolbarAction::Save;
                break;
            }
            default: return false;
        }
        if (!key.IsRepeat())
            QueueSceneAction(action);
        return true;
    }

    void EditorUILayer::QueueSceneAction(const SceneToolbarAction action, const Limen::AssetID sceneAsset) noexcept
    {
        // 原生对话框会运行自己的事件循环，等 GLFW 回调返回后再处理。
        if (m_Session && !m_RunningSceneAction && m_PendingSceneAction == SceneToolbarAction::None)
        {
            m_PendingSceneAction = action;
            m_PendingSceneAsset = sceneAsset;
        }
    }

    void EditorUILayer::ActivateSelectedAsset()
    {
        const auto id = m_Session->GetSelectedAssetID();
        Limen::AssetRecord record;
        if (m_Session->GetAssetRegistry().TryFind(id, record) &&
            DescribeContentAssetPath(record.LogicalPath).Kind == ContentAssetKind::Scene)
        {
            QueueSceneAction(SceneToolbarAction::Open, id);
            return;
        }
        ShowStatus("Asset editor is not implemented yet.", EditorStatusKind::Info);
    }

    std::filesystem::path EditorUILayer::PickSceneToOpen(const Limen::AssetID sceneAsset)
    {
        if (sceneAsset.isValid())
        {
            Limen::AssetRecord record;
            if (!m_Session->GetAssetRegistry().TryFind(sceneAsset, record) ||
                record.Status != Limen::AssetRecordStatus::Present)
            {
                ShowStatus("Scene asset is unavailable.", EditorStatusKind::Error);
                return {};
            }

            const std::u8string logicalPath(record.LogicalPath.begin(), record.LogicalPath.end());
            return m_Session->GetConfiguration().AssetRootAbsolute / std::filesystem::path(logicalPath);
        }

        std::filesystem::path path;
        const auto result = ChooseSceneToOpen(GetSceneDialogDirectory(), path);
        if (result == EditorSceneDialogResult::Unavailable)
            ShowStatus("Scene file dialog is unavailable.", EditorStatusKind::Error);
        return result == EditorSceneDialogResult::Accepted ? path : std::filesystem::path{};
    }

    std::filesystem::path EditorUILayer::GetSceneDialogDirectory() const
    {
        const auto directory = m_Session->GetScenePath().parent_path();
        return directory.empty() ? m_Session->GetConfiguration().AssetRootAbsolute : directory;
    }

    bool EditorUILayer::SaveActiveScene(const bool saveAs)
    {
        std::filesystem::path path;
        const bool choosePath = saveAs || m_Session->GetScenePath().empty();
        if (choosePath)
        {
            auto name = m_Session->GetScenePath().filename();
            if (name.empty())
                name = "Untitled.lscene";
            const auto result = ChooseSceneToSave(GetSceneDialogDirectory(), name, path);
            if (result == EditorSceneDialogResult::Unavailable)
                ShowStatus("Scene file dialog is unavailable.", EditorStatusKind::Error);
            if (result != EditorSceneDialogResult::Accepted)
                return false;
        }

        Limen::SceneFileError error;
        const bool saved = choosePath ? m_Session->TrySaveSceneAs(path, error) : m_Session->TrySaveScene(error);
        if (!saved)
        {
            LM_ERROR("保存场景失败: file='{}', key='{}', reason='{}'",
                     error.File.string(), error.Key, error.Message);
            ShowStatus("Cannot save scene. See log for details.", EditorStatusKind::Error);
            return false;
        }

        LM_INFO("已保存场景: file='{}'", m_Session->GetScenePath().string());
        const auto sceneLabel = m_Session->GetActiveSceneLabel();
        LoadWorkspaceFont(sceneLabel);
        ShowStatus("Saved scene " + sceneLabel, EditorStatusKind::Info);
        return true;
    }

    bool EditorUILayer::ConfirmSceneReplacement()
    {
        if (!m_Session->IsDirty())
            return true;

        switch (ConfirmUnsavedScene())
        {
            case EditorUnsavedSceneAction::Save: return SaveActiveScene(false);
            case EditorUnsavedSceneAction::Discard: return true;
            case EditorUnsavedSceneAction::Cancel: return false;
        }
        return false;
    }

    void EditorUILayer::RunSceneAction(const SceneToolbarAction action, const Limen::AssetID sceneAsset)
    {
        if (!m_DetailsPanel.CommitEdit(*m_Session))
        {
            ShowStatus("Complete or cancel the transform edit first.", EditorStatusKind::Error);
            return;
        }
        m_RunningSceneAction = true;
        // 模态面板可能接收原窗口的鼠标释放事件，先退出当前交互。
        m_ViewportPanel.CancelNavigation();
        static_cast<void>(m_ContentBrowser.EndDrag());
        ResetDragPreview();
        m_IsDraggingContentSplitter = false;
        m_IsDraggingDetailsSplitter = false;

        switch (action)
        {
            case SceneToolbarAction::New:
                if (ConfirmSceneReplacement())
                {
                    m_Session->NewScene();
                    m_Outliner.SetSelectionAnchor({});
                    ShowStatus("New scene.", EditorStatusKind::Info);
                }
                break;
            case SceneToolbarAction::Open:
            {
                const auto path = PickSceneToOpen(sceneAsset);
                if (!path.empty() && ConfirmSceneReplacement())
                    OpenSceneFromPath(path);
                break;
            }
            case SceneToolbarAction::Save:
            case SceneToolbarAction::SaveAs:
                static_cast<void>(SaveActiveScene(action == SceneToolbarAction::SaveAs));
                break;
            case SceneToolbarAction::None:
                break;
        }
        m_RunningSceneAction = false;
    }
}
