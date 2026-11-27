//
// Created by chenlong on 2026/10/14.
//
#pragma once
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <string>
#include <utility>

#include "Render/EditorUIRenderer.h"
#include "Render/EditorAssetSpawner.h"
#include "Render/EditorPrimitiveSpawner.h"
#include "Render/EditorSceneContent.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/Application/Layer.h"
#include "Limen/Core/Log.h"
#include "Limen/Application/Window.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"
#include "Limen/Application/Application.h"
#include "Limen/Events/KeyEvent.h"
#include "Limen/Math/AxisAlignedBoundingBox.h"
#include "Limen/Scene/SceneRenderer.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "UI/EditorUILayout.h"
#include "UI/EditorStatusMessage.h"
#include "UI/EditorTheme.h"
#include "Project/EditorSession.h"
#include "UI/Panels/ContentBrowserPanel.h"
#include "UI/Panels/DetailsPanel.h"
#include "UI/Panels/OutlinerPanel.h"
#include "UI/Panels/PlacementToolbarPanel.h"
#include "UI/Panels/SceneViewportPanel.h"
#include "UI/Panels/SceneToolbarPanel.h"

namespace LimenEditor
{
    // 编辑器专用 UI Layer：管理当前单窗口占位布局及右侧分隔条的交互状态。
    class EditorUILayer final : public Limen::Layer
    {
    public:
        explicit EditorUILayer(Limen::Ref<EditorSession> session = {})
            : Layer("Limen Editor"), m_Session(std::move(session))
        {
        }

        void OnAttach() override
        {
            if (!m_UIRenderer.Initialize())
                LM_ERROR("Editor UI renderer initialization failed");

            if (!m_ViewportPanel.Init())
                LM_ERROR("Editor viewport initialization failed");

            LoadWorkspaceFont();
            if (!m_SceneContent.Initialize())
                LM_ERROR("Editor scene content initialization failed");
        }

        // 应用在 OnAttach 返回后调用，此时模型和内置几何可以使用当前图形 Context。
        void OpenSceneFromPath(const std::filesystem::path &path);
        void QueueSceneAction(SceneToolbarAction action, Limen::AssetID sceneAsset = {}) noexcept;

        bool DeleteSelection()
        {
            return m_Session && !m_RunningSceneAction && m_Session->DeleteSelectedObjects();
        }

        // 每帧把会话场景渲染到与中央区域像素尺寸一致的离屏目标
        void OnUpdate(Limen::DeltaTime &deltaTime) override
        {
            const auto action = std::exchange(m_PendingSceneAction, SceneToolbarAction::None);
            const auto sceneAsset = std::exchange(m_PendingSceneAsset, Limen::AssetID{});
            if (action != SceneToolbarAction::None && m_Session)
                RunSceneAction(action, sceneAsset);

            if (!m_Session || !m_ViewportPanel.IsReady())
                return;

            const Limen::Window &window = Limen::Application::GetApp().GetWindow();
            const auto layout = CalculateWorkspaceLayout(window);

            m_ViewportPanel.Update(*m_Session, window, deltaTime, layout.Center);
        }

        void OnDetach() override
        {
            //保证视口 GPU 资源在 Context 销毁前释放，避免把销毁顺序交给成员析构
            m_ViewportPanel.Shutdown();
            m_FontAtlas.reset();
            m_UIRenderer.Shutdown();
        }

        // window 是本次绘制目标的非拥有引用；窗口及其图形 Context 由 Application 管理。
        void OnUIRender(Limen::Window &window) override;

        // event 是 Application 转发给本 Layer 的非拥有事件引用。
        void OnEvent(Limen::Event &event) override
        {
            if (m_RunningSceneAction)
                return;

            // dispatcher 只在本次事件调用期间存在，用于按事件类型调用对应回调。
            Limen::EventDispatcher dispatcher(event);

            dispatcher.Dispatch<Limen::KeyTypedEvent>(
                [this](const Limen::KeyTypedEvent &key)
                {
                    return m_DetailsPanel.OnKeyTyped(key.GetCodepoint());
                });
            dispatcher.Dispatch<Limen::KeyPressedEvent>(
                [this](const Limen::KeyPressedEvent &key)
                {
                    if (!m_Session)
                        return false;
                    if (HandleSceneShortcut(key))
                        return true;
                    return m_DetailsPanel.OnKeyPressed(*m_Session, key.GetKeyCode()) || HandleWorkspaceShortcut(key);
                });
            if (event.IsHandled())
                return;

            // Place 工具区位于顶栏；按下后立即生成本帧唯一的放置命令。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                    {
                        return false;
                    }

                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (m_DetailsPanel.OnMousePressed(*m_Session, x, y))
                        return true;
                    if (HandleWorkspaceClick(x, y))
                        return true;
                    const auto sceneAction = m_SceneToolbar.HitTest(x, y);
                    if (sceneAction != SceneToolbarAction::None)
                    {
                        QueueSceneAction(sceneAction);
                        return true;
                    }

                    const PlacementToolbarAction action =
                        m_PlacementToolbar.OnMousePressed(
                            x,
                            y,
                            std::chrono::steady_clock::now());
                    if (action == PlacementToolbarAction::PlaceCube)
                    {
                        static_cast<void>(PlaceCube());
                        return true;
                    }
                    if (action == PlacementToolbarAction::PlacePointLight)
                    {
                        static_cast<void>(PlacePointLight());
                        return true;
                    }
                    return false;
                });
            if (event.IsHandled())
                return;

            dispatcher.Dispatch<Limen::MouseScrolledEvent>(
                [this](const Limen::MouseScrolledEvent &scroll)
                {
                    if (!m_Session || m_ContentBrowser.IsDraggingAsset())
                        return false;
                    const auto [x, y] = Limen::Input::GetMousePos();
                    return m_ContentBrowser.OnMouseScrolled(x, y, scroll.GetYOffset());
                });
            if (event.IsHandled())
                return;

            // 工具栏与资源抽屉先消费自己的输入，随后才进入场景视口。
            if ((m_WorkspaceLayout.Center.Width > 0.0f && m_ViewportPanel.OnEvent(event)) || event.IsHandled())
                return;

            // 视口左键：射线拾取对象；Cmd/Ctrl 支持增选。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (!m_WorkspaceLayout.Center.Contains(x, y))
                        return false;
                    const bool toggle =
                        Limen::Input::IsKeyPressed(Limen::KeyCode::LeftSuper) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::RightSuper) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::LeftControl) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::RightControl);

                    return m_ViewportPanel.PickAndSelect(
                        *m_Session, x, y, toggle);
                });
            if (event.IsHandled())
                return;

            // 抽屉左键命中资源卡片或 Place 操作。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    const auto [x, y] = Limen::Input::GetMousePos();
                    const ContentBrowserAction action =
                        m_ContentBrowser.OnMousePressed(*m_Session, x, y);

                    if (action == ContentBrowserAction::ActivateAsset)
                    {
                        ActivateSelectedAsset();
                        return true;
                    }
                    if (action == ContentBrowserAction::PlaceAsset)
                        return PlaceSelectedAsset();

                    return action == ContentBrowserAction::SelectionChanged ||
                           action == ContentBrowserAction::FolderChanged;
                });
            if (event.IsHandled())
                return;

            // Outliner 只处理已有对象的选择。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    const auto [x, y] = Limen::Input::GetMousePos();

                    // macOS 使用 Command，Windows/Linux 使用 Ctrl；统一接受两者。
                    if (!m_WorkspaceLayout.Outliner.Contains(x, y))
                        return false;
                    const bool toggleDown =
                        Limen::Input::IsKeyPressed(Limen::KeyCode::LeftSuper) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::RightSuper) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::LeftControl) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::RightControl);
                    const bool rangeDown =
                        Limen::Input::IsKeyPressed(Limen::KeyCode::LeftShift) ||
                        Limen::Input::IsKeyPressed(Limen::KeyCode::RightShift);

                    const OutlinerPanelAction action =
                        m_Outliner.OnMousePressed(
                            *m_Session,
                            x,
                            y,
                            toggleDown,
                            rangeDown);

                    return action == OutlinerPanelAction::SelectionChanged;
                });
            if (event.IsHandled())
                return;

            dispatcher.Dispatch<Limen::MouseMovedEvent>(
                [this](const Limen::MouseMovedEvent &move)
                {
                    const bool wasDragging =
                        m_ContentBrowser.IsDraggingAsset();
                    m_ContentBrowser.OnMouseMoved(
                        move.GetX(),
                        move.GetY());
                    if (!wasDragging &&
                        m_ContentBrowser.IsDraggingAsset())
                        BeginDragPreview();
                    return false;
                });
            if (event.IsHandled())
                return;

            dispatcher.Dispatch<Limen::MouseScrolledEvent>(
                [this](const Limen::MouseScrolledEvent &scroll)
                {
                    if (!m_ContentBrowser.IsDraggingAsset())
                        return false;

                    constexpr float distanceStep = 0.5f;
                    constexpr float minDistance = 1.0f;
                    constexpr float maxDistance = 20.0f;
                    m_PlacementDistance = std::clamp(
                        m_PlacementDistance -
                        scroll.GetYOffset() * distanceStep,
                        minDistance,
                        maxDistance);
                    return true;
                });
            if (event.IsHandled())
                return;

            // Content Browser 拖到视口后释放：按鼠标射线生成对象。
            dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
                [this](const Limen::MouseButtonReleasedEvent &release)
                {
                    if (release.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    if (!m_ContentBrowser.EndDrag())
                        return false;
                    ResetDragPreview();

                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (m_ViewportPanel.ContainsPoint(x, y))
                        PlaceSelectedAssetAt(x, y);
                    else
                        ShowStatus(
                            "Drop cancelled: release inside the Scene viewport.",
                            EditorStatusKind::Error);
                    return true;
                });
            if (event.IsHandled())
                return;

            dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
                [this](const Limen::MouseButtonReleasedEvent &release)
                {
                    if (release.GetButton() != Limen::MouseButton::Left)
                        return false;

                    if (m_IsDraggingContentSplitter)
                    {
                        ResizeContentDrawer(Limen::Input::GetMouseY());
                        m_IsDraggingContentSplitter = false;
                        return true;
                    }
                    if (m_IsDraggingDetailsSplitter)
                    {
                        ResizeDetailsDock(Limen::Input::GetMouseY());
                        m_IsDraggingDetailsSplitter = false;
                        return true;
                    }
                    return false;
                });
            if (event.IsHandled())
                return;

            dispatcher.Dispatch<Limen::WindowResizeEvent>(
                [this](const Limen::WindowResizeEvent &)
                {
                    ResetWorkspaceHitAreas();
                    m_SceneToolbar.Reset();
                    m_PlacementToolbar.UpdateLayout({}, 0.0f);
                    m_IsDraggingContentSplitter = false;
                    m_IsDraggingDetailsSplitter = false;
                    m_ViewportPanel.CancelNavigation();
                    return false;
                });

            // Delete/Backspace：删除当前全部选中对象
            dispatcher.Dispatch<Limen::KeyPressedEvent>(
                [this](const Limen::KeyPressedEvent &keyEvent)
                {
                    if (!m_Session)
                        return false;

                    const Limen::KeyCode key = keyEvent.GetKeyCode();
                    if (key != Limen::KeyCode::Delete && key != Limen::KeyCode::Backspace)
                        return false;
                    return DeleteSelection();
                }
            );
        }

    private:
        bool HandleSceneShortcut(const Limen::KeyPressedEvent &key);
        bool HandleWorkspaceShortcut(const Limen::KeyPressedEvent &key);
        bool HandleWorkspaceClick(float x, float y);
        [[nodiscard]] EditorUILayoutResult CalculateWorkspaceLayout(const Limen::Window &window) const;
        void ResetWorkspaceHitAreas();
        void ToggleContentDrawer();
        void ToggleExpandedViewport();
        void ResizeContentDrawer(float mouseY);
        void ResizeDetailsDock(float mouseY);
        void DrawWorkspaceHeader(const Limen::Window &window, const UIRect &top);
        void DrawViewportToolbar(const Limen::Window &window, const UIRect &rect);
        void DrawWorkspaceStatusBar(const Limen::Window &window, const UIRect &rect);
        void DrawAssetDragPreview(const Limen::Window &window) const;
        void LoadWorkspaceFont(std::string_view sceneLabel = {});
        void ActivateSelectedAsset();
        void RunSceneAction(SceneToolbarAction action, Limen::AssetID sceneAsset);
        [[nodiscard]] std::filesystem::path PickSceneToOpen(Limen::AssetID sceneAsset);
        [[nodiscard]] std::filesystem::path GetSceneDialogDirectory() const;
        [[nodiscard]] bool SaveActiveScene(bool saveAs);
        [[nodiscard]] bool ConfirmSceneReplacement();

        bool PlaceCube()
        {
            Limen::SceneTransform placementTransform;
            if (!m_ViewportPanel.TryGetPlacementTransform(
                    placementTransform,
                    m_PlacementDistance))
            {
                ShowStatus(
                    "Cannot place Cube: Scene viewport is not ready.",
                    EditorStatusKind::Error);
                return false;
            }

            Limen::LArray<Limen::SceneObjectID> created;
            std::string spawnError;
            if (!m_PrimitiveSpawner.SpawnCube(
                    *m_Session,
                    m_SceneContent,
                    placementTransform,
                    created,
                    &spawnError) ||
                created.getSize() == 0)
            {
                ShowStatus(
                    spawnError.empty()
                        ? "Cannot place Cube."
                        : spawnError,
                    EditorStatusKind::Error);
                return false;
            }

            m_Outliner.SetSelectionAnchor(
                created[created.getSize() - 1]);
            ShowStatus("Placed Cube.", EditorStatusKind::Info);
            return true;
        }

        bool PlacePointLight()
        {
            Limen::SceneTransform placementTransform;
            if (!m_ViewportPanel.TryGetPlacementTransform(
                    placementTransform,
                    m_PlacementDistance))
            {
                ShowStatus(
                    "Cannot place Point Light: Scene viewport is not ready.",
                    EditorStatusKind::Error);
                return false;
            }

            Limen::LArray<Limen::SceneObjectID> created;
            std::string spawnError;
            if (!m_PrimitiveSpawner.SpawnPointLight(
                    *m_Session,
                    placementTransform,
                    created,
                    &spawnError) ||
                created.getSize() == 0)
            {
                ShowStatus(
                    spawnError.empty()
                        ? "Cannot place Point Light."
                        : spawnError,
                    EditorStatusKind::Error);
                return false;
            }

            m_Outliner.SetSelectionAnchor(
                created[created.getSize() - 1]);
            ShowStatus("Placed Point Light.", EditorStatusKind::Info);
            return true;
        }

        // 只允许显式放置当前选中的资产；不提供隐藏的默认对象兜底。
        bool PlaceSelectedAsset()
        {
            Limen::SceneTransform placementTransform;
            if (!m_ViewportPanel.TryGetPlacementTransform(placementTransform))
            {
                LM_WARN("Editor place: viewport placement transform is unavailable");
                ShowStatus(
                    "Cannot place: Scene viewport is not ready.",
                    EditorStatusKind::Error);
                return false;
            }
            return PlaceSelectedAssetWithTransform(placementTransform);
        }

        bool PlaceSelectedAssetAt(const float x, const float y)
        {
            Limen::SceneTransform placementTransform;
            if (!m_ViewportPanel.TryGetPlacementTransformAt(
                    x,
                    y,
                    placementTransform,
                    m_PlacementDistance))
            {
                LM_WARN("Editor place: drop position is outside the viewport");
                ShowStatus(
                    "Cannot place: drop position is outside the Scene viewport.",
                    EditorStatusKind::Error);
                return false;
            }
            return PlaceSelectedAssetWithTransform(placementTransform);
        }

        bool PlaceSelectedAssetWithTransform(
            const Limen::SceneTransform &placementTransform)
        {
            if (!m_Session)
            {
                LM_WARN("Editor place: session is unavailable");
                ShowStatus(
                    "Cannot place: Editor session is unavailable.",
                    EditorStatusKind::Error);
                return false;
            }
            if (!m_Session->GetSelectedAssetID().isValid())
            {
                LM_WARN("Editor place: no asset is selected");
                ShowStatus(
                    "Cannot place: no asset is selected.",
                    EditorStatusKind::Error);
                return false;
            }

            Limen::LArray<Limen::SceneObjectID> created;
            std::string spawnError;
            if (!m_AssetSpawner.SpawnSelectedAsset(
                    *m_Session,
                    m_SceneContent,
                    placementTransform,
                    created,
                    &spawnError))
            {
                LM_WARN("Editor place: selected asset could not be instantiated");
                ShowStatus(
                    spawnError.empty()
                        ? "Cannot place the selected asset."
                        : spawnError,
                    EditorStatusKind::Error);
                return false;
            }

            if (created.getSize() == 0)
            {
                LM_WARN("Editor place: spawner returned no scene object");
                ShowStatus(
                    "Cannot place: spawner returned no scene object.",
                    EditorStatusKind::Error);
                return false;
            }

            // 新建对象成为当前选择，并作为后续 Shift 范围的起点。
            m_Outliner.SetSelectionAnchor(created[created.getSize() - 1]);
            LM_INFO("Editor place: created {} scene object(s)", created.getSize());
            ShowStatus(
                "Placed the selected asset.",
                EditorStatusKind::Info);
            return true;
        }

        void BeginDragPreview()
        {
            m_PlacementDistance = 4.0f;
            m_HasDragPreviewBounds = false;
            m_DragPreviewLabel.clear();

            if (!m_Session ||
                !m_Session->GetSelectedAssetID().isValid())
                return;

            Limen::AssetRecord record;
            if (m_Session->GetAssetRegistry().TryFind(
                    m_Session->GetSelectedAssetID(),
                    record))
            {
                m_DragPreviewLabel.assign(
                    record.LogicalPath.data(),
                    record.LogicalPath.size());
            }

            Limen::AxisAlignedBoundingBox bounds;
            std::string previewError;
            if (m_AssetSpawner.TryGetSelectedAssetBounds(
                    *m_Session,
                    bounds,
                    &previewError))
            {
                m_DragPreviewBounds = bounds;
                m_HasDragPreviewBounds = true;
                return;
            }

            if (!previewError.empty())
                ShowStatus(previewError, EditorStatusKind::Error);
            m_DragPreviewLabel = "Preview unavailable";
        }

        void ResetDragPreview() noexcept
        {
            m_HasDragPreviewBounds = false;
            m_DragPreviewLabel.clear();
        }

        void ShowStatus(
            std::string message,
            const EditorStatusKind kind)
        {
            if (message.empty())
                return;
            LM_INFO("Editor status: {}", message);
            m_StatusMessage.Show(
                std::move(message),
                kind,
                std::chrono::steady_clock::now());
        }

        //region attributes definitions

        // 本 Layer 按值拥有 UI 绘制器；OnAttach 初始化，OnDetach 在 Context 销毁前关闭。
        EditorUIRenderer m_UIRenderer;
        float m_ContentHeightRatio = 0.24f;
        float m_DetailsSplitRatio = kDefaultEditorRightSplitRatio;
        bool m_ContentDrawerOpen = true;
        bool m_ViewportExpanded = false;
        bool m_IsDraggingContentSplitter = false;
        bool m_IsDraggingDetailsSplitter = false;
        // 命中使用最近一次完成绘制的区域。
        EditorUILayoutResult m_WorkspaceLayout{};
        UIRect m_ContentToggleRect{};
        UIRect m_FrameSelectionRect{};
        UIRect m_ExpandViewportRect{};

        Limen::Ref<EditorSession> m_Session;

        SceneViewportPanel m_ViewportPanel;

        // Editor 统一主题色；后续所有面板只从这里取颜色和间距。
        EditorTheme m_Theme;
        // 底部资源抽屉拥有卡片布局与资产选择交互。
        ContentBrowserPanel m_ContentBrowser;
        // 拖拽放置沿相机视线的距离，滚轮可调。
        float m_PlacementDistance = 4.0f;
        Limen::AxisAlignedBoundingBox m_DragPreviewBounds;
        std::string m_DragPreviewLabel;
        bool m_HasDragPreviewBounds = false;

        // Outliner 位于右侧上方，持有行布局与 Shift 锚点。
        OutlinerPanel m_Outliner;

        // 引擎默认字体的图集；面板文字只借用，生命周期由本 Layer 管理。
        Limen::Scope<Limen::FontAtlas> m_FontAtlas;

        // Editor 默认立方体等引导内容；GPU 资源由渲染层持有。
        EditorSceneContent m_SceneContent;
        // 资产选择到可渲染对象的转换器。
        EditorAssetSpawner m_AssetSpawner;
        // 内置 Cube 与 PointLight 的 Authoring 创建入口。
        EditorPrimitiveSpawner m_PrimitiveSpawner;
        // 顶栏 Place 工具区的按钮布局与输入命中。
        PlacementToolbarPanel m_PlacementToolbar;
        SceneToolbarPanel m_SceneToolbar;
        SceneToolbarAction m_PendingSceneAction = SceneToolbarAction::None;
        Limen::AssetID m_PendingSceneAsset{};
        bool m_RunningSceneAction = false;

        // 右侧 Details 借用会话，数字输入存在面板草稿中。
        DetailsPanel m_DetailsPanel;
        EditorStatusMessage m_StatusMessage;
        //endregion
    };
}
