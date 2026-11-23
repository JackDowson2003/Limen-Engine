//
// Created by chenlong on 2026/10/14.
//
#pragma once
#include <algorithm>
#include <cstdio>
#include <string>
#include <utility>

#include "Render/EditorUIRenderer.h"
#include "Render/EditorAssetSpawner.h"
#include "Render/EditorSceneContent.h"
#include "Limen/Platform/PlatformPaths.h"
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
#include "UI/Panels/SceneViewportPanel.h"

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

            // 引擎默认字体：从内置资产根解析路径，失败只记日志、不阻断启动。
            Limen::LString fontError;
            std::filesystem::path fontPath;
            if (Limen::TryGetBuiltInAssetRoot(fontPath, fontError))
            {
                fontPath /= "fonts/NotoSansCJKsc-Regular.otf";
                m_FontAtlas = Limen::CreateScope<Limen::FontAtlas>(1024, 24.0f);
                if (m_FontAtlas->LoadFontFile(fontPath))
                {
                    std::string uiCharacters;
                    for (char c = 32; c < 127; ++c)
                        uiCharacters.push_back(c);
                    uiCharacters += "场景视图对象选择内容浏览器创建详情无";
                    m_FontAtlas->AddText(uiCharacters);
                    if (!m_FontAtlas->Build())
                        LM_ERROR("Editor UI: font atlas build failed");
                } else
                {
                    LM_ERROR("Editor UI: default font load failed: {}",
                             fontPath.string());
                }
            } else
            {
                LM_ERROR("Editor UI: built-in asset root unavailable: {}",
                         fontError);
            }
            if (!m_SceneContent.Initialize())
                LM_ERROR("Editor scene content initialization failed");
        }

        // 每帧把会话场景渲染到与中央区域像素尺寸一致的离屏目标
        void OnUpdate(Limen::DeltaTime &deltaTime) override
        {
            if (!m_Session || !m_ViewportPanel.IsReady())
                return;

            const Limen::Window &window = Limen::Application::GetApp().GetWindow();
            const EditorUILayoutResult layout = CalculateEditorUILayout(
                static_cast<float>(window.GetWindowWidth()),
                static_cast<float>(window.GetWindowHeight()),
                m_RightTopRatio);

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
        void OnUIRender(Limen::Window &window) override
        {
            // 每次都当前窗口的逻辑尺寸，普通缩放，普通缩放和非全屏最大化走同一套布局
            const float width = static_cast<float>(window.GetWindowWidth());
            const float height = static_cast<float>(window.GetWindowHeight());
            const EditorUILayoutResult &layout = CalculateEditorUILayout(width, height, m_RightTopRatio);
            // 输入命中区域必须与本次实际绘制的分隔条一致。
            m_RightSplitterRect = layout.RightSplitter;

            if (layout.Top.Width <= 0.0f || layout.Top.Height <= 0.0f)
                return;

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Top.X, layout.Top.Y},
                {layout.Top.Width, layout.Top.Height},
                m_Theme.TopBarBackground
            );

            if (m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                const auto now = std::chrono::steady_clock::now();
                const std::string_view status = m_StatusMessage.GetText(now);
                std::string header = "Limen Editor 场景 视图";
                glm::vec4 headerColor = m_Theme.TextPrimary;
                if (!status.empty())
                {
                    header += "    ";
                    header += status;
                    headerColor =
                        m_StatusMessage.GetKind(now) == EditorStatusKind::Error
                            ? m_Theme.MissingText
                            : m_Theme.ButtonText;
                }

                m_UIRenderer.DrawText(
                    window,
                    {layout.Top.X + 12.0f, layout.Top.Y + 8.0f},
                    headerColor,
                    *m_FontAtlas,
                    header
                );
            }

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Left.X, layout.Left.Y},
                {layout.Left.Width, layout.Left.Height},
                m_Theme.PanelBackground
            );
            // Content Browser 的行数据、绘制和点击由面板自己管理。
            if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_ContentBrowser.Update(*m_Session);
                m_ContentBrowser.Draw(
                    *m_Session,
                    window,
                    m_UIRenderer,
                    *m_FontAtlas,
                    m_Theme,
                    layout.Left);
            }
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Center.X, layout.Center.Y},
                {layout.Center.Width, layout.Center.Height},
                m_Theme.ViewportBackground
            );

            m_ViewportPanel.Draw(window,m_UIRenderer,layout.Center);

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightTop.X, layout.RightTop.Y},
                {layout.RightTop.Width, layout.RightTop.Height},
                m_Theme.PanelBackground
            );

            // Outliner 的行数据、多选和绘制由面板自己管理。
            if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_Outliner.Update(*m_Session);
                m_Outliner.Draw(
                    *m_Session,
                    window,
                    m_UIRenderer,
                    *m_FontAtlas,
                    m_Theme,
                    layout.RightTop);
            }


            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightSplitter.X, layout.RightSplitter.Y},
                {layout.RightSplitter.Width, layout.RightSplitter.Height},
                m_Theme.Splitter
            );
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightBottom.X, layout.RightBottom.Y},
                {layout.RightBottom.Width, layout.RightBottom.Height},
                m_Theme.PanelBackground
            );

            // Details 面板只借用绘画状态选择状态，绘制细节由面板自身负责
            if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_DetailsPanel.Draw(
                    *m_Session,
                    window,
                    m_UIRenderer,
                    *m_FontAtlas,
                    m_Theme,
                    layout.RightBottom
                );
            }

            if (m_ContentBrowser.IsDraggingAsset() &&
                m_FontAtlas &&
                m_FontAtlas->IsBuilt())
            {
                const auto [x, y] = Limen::Input::GetMousePos();
                m_UIRenderer.DrawSolidRect(
                    window,
                    {x - 3.0f, y - 3.0f},
                    {6.0f, 6.0f},
                    m_Theme.SelectionBackground);

                if (m_HasDragPreviewBounds)
                {
                    Limen::SceneTransform placementTransform;
                    if (m_ViewportPanel.TryGetPlacementTransformAt(
                            x,
                            y,
                            placementTransform,
                            m_PlacementDistance))
                    {
                        const Limen::AxisAlignedBoundingBox worldBounds =
                            m_DragPreviewBounds.Transformed(
                                placementTransform.ToMatrix());
                        glm::vec2 screenMin{};
                        glm::vec2 screenMax{};
                        if (m_ViewportPanel.TryProjectWorldBounds(
                                worldBounds,
                                screenMin,
                                screenMax))
                        {
                            glm::vec4 outlineColor =
                                m_Theme.SelectionBackground;
                            outlineColor.a = 0.7f;
                            constexpr float thickness = 2.0f;
                            const float previewWidth =
                                screenMax.x - screenMin.x;
                            const float previewHeight =
                                screenMax.y - screenMin.y;

                            m_UIRenderer.DrawSolidRect(
                                window,
                                screenMin,
                                {previewWidth, thickness},
                                outlineColor);
                            m_UIRenderer.DrawSolidRect(
                                window,
                                {screenMin.x, screenMax.y - thickness},
                                {previewWidth, thickness},
                                outlineColor);
                            m_UIRenderer.DrawSolidRect(
                                window,
                                screenMin,
                                {thickness, previewHeight},
                                outlineColor);
                            m_UIRenderer.DrawSolidRect(
                                window,
                                {screenMax.x - thickness, screenMin.y},
                                {thickness, previewHeight},
                                outlineColor);
                        }
                    }
                }

                char distanceLabel[64]{};
                std::snprintf(
                    distanceLabel,
                    sizeof(distanceLabel),
                    "  Distance: %.1f",
                    static_cast<double>(m_PlacementDistance));
                std::string label = m_DragPreviewLabel.empty()
                                        ? "Place asset"
                                        : m_DragPreviewLabel;
                label += distanceLabel;
                m_UIRenderer.DrawText(
                    window,
                    {x + 12.0f, y + 12.0f},
                    m_Theme.SelectionText,
                    *m_FontAtlas,
                    label);
            }
        }

        // event 是 Application 转发给本 Layer 的非拥有事件引用。
        void OnEvent(Limen::Event &event) override
        {
            // dispatcher 只在本次事件调用期间存在，用于按事件类型调用对应回调。
            Limen::EventDispatcher dispatcher(event);

            // 输入路由优先级：Viewport → Content Browser → Outliner →
            // Splitter → 快捷键。被高优先级区域消费后不再进入后续区域。
            if (m_ViewportPanel.OnEvent(event) || event.IsHandled())
                return;

            // 视口左键：射线拾取对象；Cmd/Ctrl 支持增选。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    const auto [x, y] = Limen::Input::GetMousePos();
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

            // Content Browser 左键：按行号找到资产记录并切换资产选择。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    const auto [x, y] = Limen::Input::GetMousePos();
                    const ContentBrowserAction action =
                        m_ContentBrowser.OnMousePressed(*m_Session, x, y);

                    // 单击选择资产；双击留给资产编辑器；放置必须走显式 Place。
                    if (action == ContentBrowserAction::ActivateAsset)
                    {
                        LM_INFO("Editor asset editor is not implemented yet");
                        ShowStatus(
                            "Asset editor is not implemented yet.",
                            EditorStatusKind::Info);
                        return true;
                    }
                    if (action == ContentBrowserAction::PlaceAsset)
                        return PlaceSelectedAsset();

                    return action == ContentBrowserAction::SelectionChanged;
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

            // press 仅提供鼠标按钮；按下位置需从当前单窗口的 Input 查询。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left)
                        return false;
                    // x、y 分别是鼠标的逻辑横、纵坐标，用于命中最近一次绘制的分隔条。
                    if (
                        const auto [x, y] = Limen::Input::GetMousePos();
                        !m_RightSplitterRect.Contains(x, y)
                    )
                        return false;
                    m_IsDraggingRightSplitter = true;
                    LM_INFO("Editor right splitter grabbed");
                    return true;
                });
            if (event.IsHandled())
                return;

            // release 仅提供鼠标按钮；这里只处理从分隔条开始的左键操作。
            dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
                [this](const Limen::MouseButtonReleasedEvent &release)
                {
                    if (release.GetButton() != Limen::MouseButton::Left || !m_IsDraggingRightSplitter)
                        return false;
                    // mouseY 是松开时光标的逻辑 Y；ReleasedEvent 本身不携带坐标。
                    const float mouseY = Limen::Input::GetMouseY();
                    // window 是 Application 当前拥有的主窗口，此处只借用它读取最新逻辑高度。
                    const auto &window = Limen::Application::GetApp().GetWindow();
                    // height 是松开时窗口内容区的逻辑高度，不是帧缓冲像素高度。
                    const float width = static_cast<float>(window.GetWindowWidth());
                    const float height = static_cast<float>(window.GetWindowHeight());
                    const EditorUILayoutResult layout = CalculateEditorUILayout(width, height, m_RightTopRatio);

                    // 右侧正文总高：右上区域、分隔条和右下区域都包含在内。
                    const float rightBodyHeight =
                            layout.RightTop.Height
                            + layout.RightSplitter.Height
                            + layout.RightBottom.Height;

                    if (rightBodyHeight > 0.0f)
                    {
                        // 鼠标相对右侧正文顶部的位置，用于决定下一帧的上下比例。
                        const float mouseOffset = mouseY - layout.RightTop.Y;
                        m_RightTopRatio = std::clamp(
                            mouseOffset / rightBodyHeight,
                            0.000001f,
                            0.999999f
                        );
                    }
                    LM_INFO("m_RightTopRatio = {} ", m_RightTopRatio);

                    m_IsDraggingRightSplitter = false;
                    LM_INFO("Editor right splitter released");
                    return true;
                });
            if (event.IsHandled())
                return;

            // 尺寸变化后，上一次绘制的逻辑坐标命中区域立即失效；下次 OnUIRender 会重建。
            dispatcher.Dispatch<Limen::WindowResizeEvent>(
                [this](const Limen::WindowResizeEvent &resize)
                {
                    m_RightSplitterRect = {};
                    if (resize.GetWidth() != 0 && resize.GetHeight() != 0)
                        return false;

                    if (m_IsDraggingRightSplitter)
                        LM_INFO("Editor right splitter drag cancelled: framebuffer size is zero");

                    m_ViewportPanel.CancelNavigation();
                    return false;
                }
            );

            // Delete/Backspace：删除当前全部选中对象
            dispatcher.Dispatch<Limen::KeyPressedEvent>(
                [this](const Limen::KeyPressedEvent &keyEvent)
                {
                    if (!m_Session)
                        return false;

                    const Limen::KeyCode key = keyEvent.GetKeyCode();
                    if (key != Limen::KeyCode::Delete && key != Limen::KeyCode::Backspace)
                        return false;
                    return m_Session->DeleteSelectedObjects();
                }
            );
        }

    private:
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
        // 右上面板的绘制比例；绘制时乘以扣除分隔条后的 availableHeight。
        float m_RightTopRatio = 0.5f;
        // 最近一次 OnUIRender 实际绘制的分隔条命中区域；每次绘制前清空并重建，用于鼠标事件的判断
        UIRect m_RightSplitterRect{};

        // 左键是否从分隔条开始按下，等待松开时更新比例；当前并非实时拖动。
        bool m_IsDraggingRightSplitter = false;

        Limen::Ref<EditorSession> m_Session;

        SceneViewportPanel m_ViewportPanel;

        // Editor 统一主题色；后续所有面板只从这里取颜色和间距。
        EditorTheme m_Theme;
        // 左侧资产浏览器面板：持有行布局与资产选择交互。
        ContentBrowserPanel m_ContentBrowser;
        // 拖拽放置沿相机视线的距离，滚轮可调。
        float m_PlacementDistance = 4.0f;
        Limen::AxisAlignedBoundingBox m_DragPreviewBounds;
        std::string m_DragPreviewLabel;
        bool m_HasDragPreviewBounds = false;

        // 右上 Outliner：持有行布局与 Shift 锚点。
        OutlinerPanel m_Outliner;

        // 引擎默认字体的图集；面板文字只借用，生命周期由本 Layer 管理。
        Limen::Scope<Limen::FontAtlas> m_FontAtlas;

        // Editor 默认立方体等引导内容；GPU 资源由渲染层持有。
        EditorSceneContent m_SceneContent;
        // 资产选择到可渲染对象的转换器。
        EditorAssetSpawner m_AssetSpawner;

        // 右侧 Details 面板：无状态，只读取会话选择。
        DetailsPanel m_DetailsPanel;
        EditorStatusMessage m_StatusMessage;
        //endregion
    };
}
