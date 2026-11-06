//
// Created by chenlong on 2026/10/14.
//
#pragma once
#include <algorithm>
#include <utility>

#include "Render/EditorUIRenderer.h"
#include "Limen/Platform/PlatformPaths.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/Application/Layer.h"
#include "Limen/Core/Log.h"
#include "Limen/Application/Window.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"
#include "Limen/Application/Application.h"
#include "Limen/Asset/AssetManager.h"
#include "Limen/Scene/SceneRenderer.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "UI/EditorUILayout.h"
#include "Project/EditorSession.h"

namespace LimenEditor
{
    // 编辑器专用 UI Layer：管理当前单窗口占位布局及右侧分隔条的交互状态。
    class EditorUILayer final : public Limen::Layer
    {
    public:
        explicit EditorUILayer(Limen::Ref<EditorSession> session = {})
            : Layer("Limen Editor"), m_Session(std::move(session)),
        m_ViewportCameraController(45.0f, 1.0f, 0.1f, 100.f)
        {
        }

        void OnAttach() override
        {
            if (!m_UIRenderer.Initialize())
                LM_ERROR("Editor UI renderer initialization failed");

            Limen::SceneRendererSpecification viewportSpec;
            viewportSpec.Width = 1;
            viewportSpec.Height = 1;
            viewportSpec.DebugName = "Editor Scene Viewport";
            m_ViewportRenderer = Limen::CreateScope<Limen::SceneRenderer>(viewportSpec);
            m_ViewportCameraController.GetCamera().SetPosition({0.0f, 0.0f, 0.5f});

            // 引擎默认字体：从内置资产根解析路径，失败只记日志、不阻断启动。
            Limen::LString fontError;
            std::filesystem::path fontPath;
            if (Limen::TryGetBuiltInAssetRoot(fontPath, fontError))
            {
                fontPath /= "fonts/NotoSansCJKsc-Regular.otf";
                m_FontAtlas = Limen::CreateScope<Limen::FontAtlas>(1024, 24.0f);
                if (m_FontAtlas->LoadFontFile(fontPath))
                {
                    m_FontAtlas->AddText("Limen Editor 场景 视图 对象 选择");
                    if (!m_FontAtlas->Build())
                        LM_ERROR("Editor UI: font atlas build failed");
                }
                else
                {
                    LM_ERROR("Editor UI: default font load failed: {}",
                             fontPath.string());
                }
            }
            else
            {
                LM_ERROR("Editor UI: built-in asset root unavailable: {}",
                         fontError);
            }

        }

        // 每帧把会话场景渲染到与中央区域像素尺寸一致的离屏目标
        void OnUpdate(Limen::DeltaTime& deltaTime) override
        {
            if (!m_Session || !m_ViewportRenderer)
                return;

            const Limen::Window &window = Limen::Application::GetApp().GetWindow();
            const float logicalWidth = static_cast<float>(window.GetWindowWidth());
            const float logicalHeight = static_cast<float>(window.GetWindowHeight());
            if (logicalWidth <= 0.0f || logicalHeight <= 0.0f)
                return;

            const EditorUILayoutResult layout =
                    CalculateEditorUILayout(logicalWidth, logicalHeight, m_RightTopRatio);
            m_CenterRect = layout.Center;

            // 光标在中央区域内，或从视口开始的右键拖拽进行中，才允许相机输入。
            const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
            m_IsViewportHovered = m_CenterRect.Contains(mouseX, mouseY);
            m_ViewportCameraController.SetMouseLookEnabled(
                m_IsViewportHovered || m_IsNavigatingViewport);
            m_ViewportCameraController.OnUpdate(deltaTime);

            const auto [Width, Height] = CalculateViewportPixelSize(
                logicalWidth, logicalHeight, layout.Center,
                window.GetWidth(), window.GetHeight());

            const auto &spec = m_ViewportRenderer->GetSpecification();
            if (spec.Width != Width || spec.Height != Height)
            {
                m_ViewportRenderer->Resize(Width, Height);
                m_ViewportCameraController.GetCamera().SetAspectRatio(
                    static_cast<float>(Width) /
                    static_cast<float>(Height));
            }

            m_ViewportRenderer->Render(
                m_Session->GetScene(),
                m_ViewportCameraController.GetCamera());
        }

        void OnDetach() override
        {
            //保证视口 GPU 资源在 Context 销毁前释放，避免把销毁顺序交给成员析构
            m_ViewportRenderer.reset();
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
                {0.17f, 0.18f, 0.20f, 1.0f}
            );
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Left.X, layout.Left.Y},
                {layout.Left.Width, layout.Left.Height},
                {0.13f, 0.14f, 0.16f, 1.0f}
            );
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Center.X, layout.Center.Y},
                {layout.Center.Width, layout.Center.Height},
                {0.08f, 0.09f, 0.11f, 1.0f}
            );
            if (m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_UIRenderer.DrawText(
                    window,
                    {layout.Top.X + 12.0f, layout.Top.Y + 8.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    *m_FontAtlas,
                    "Limen Editor 场景 视图"
                );
            }

            const std::uintptr_t finalHandle =
                    m_ViewportRenderer
                        ? m_ViewportRenderer->GetFinalColorAttachmentHandle()
                        : 0;
            if (finalHandle != 0)
            {
                m_UIRenderer.DrawNativeTextureRect(
                    window,
                    {layout.Center.X, layout.Center.Y},
                    {layout.Center.Width,layout.Center.Height},
                    finalHandle
                );
            }

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightTop.X, layout.RightTop.Y},
                {layout.RightTop.Width, layout.RightTop.Height},
                {0.15f, 0.16f, 0.18f, 1.0f}
            );
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightSplitter.X, layout.RightSplitter.Y},
                {layout.RightSplitter.Width, layout.RightSplitter.Height},
                {0.35f, 0.37f, 0.42f, 1.0f}
            );
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightBottom.X, layout.RightBottom.Y},
                {layout.RightBottom.Width, layout.RightBottom.Height},
                {0.21f, 0.22f, 0.24f, 1.0f}
            );
        }

                // event 是 Application 转发给本 Layer 的非拥有事件引用。
        void OnEvent(Limen::Event &event) override
        {
            // 视口内滚轮交给相机控制器：飞行时调速，非飞行时沿视线推拉缩放。
            if (event.GetEventType() == Limen::EventType::MouseScrolled &&
                m_IsViewportHovered)
                m_ViewportCameraController.OnEvent(event);

            // dispatcher 只在本次事件调用期间存在，用于按事件类型调用对应回调。
            Limen::EventDispatcher dispatcher(event);

            // 右键（环视/飞行）或中键（平移）从中央区域按下时，视口接管相机导航。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    const auto button = press.GetButton();
                    if (button != Limen::MouseButton::Right &&
                        button != Limen::MouseButton::Middle)
                        return false;
                    // x、y 是鼠标的逻辑横、纵坐标，用于命中最近一次布局的中央区域。
                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (!m_CenterRect.Contains(x, y))
                        return false;
                    m_IsNavigatingViewport = true;
                    return true;
                });

            dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
                [this](const Limen::MouseButtonReleasedEvent &release)
                {
                    const auto button = release.GetButton();
                    if ((button != Limen::MouseButton::Right &&
                         button != Limen::MouseButton::Middle) ||
                        !m_IsNavigatingViewport)
                        return false;
                    m_IsNavigatingViewport = false;
                    return true;
                });

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

            // 尺寸变化后，上一次绘制的逻辑坐标命中区域立即失效；下次 OnUIRender 会重建。
            dispatcher.Dispatch<Limen::WindowResizeEvent>(
                [this](const Limen::WindowResizeEvent &resize)
                {
                    m_RightSplitterRect = {};
                    if (resize.GetWidth() != 0 && resize.GetHeight() != 0)
                        return false;

                    if (m_IsDraggingRightSplitter)
                        LM_INFO("Editor right splitter drag cancelled: framebuffer size is zero");

                    m_IsDraggingRightSplitter = false;

                    // 零尺寸帧缓冲也会打断视口导航；恢复光标，避免右键环视锁定残留。
                    m_IsNavigatingViewport = false;
                    m_IsViewportHovered = false;
                    m_ViewportCameraController.SetMouseLookEnabled(false);
                    return false;
                }
            );
        }

    private:
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

        // 视口独占的离屏渲染器与相机；场景来自会话，只借用。
        Limen::Scope<Limen::SceneRenderer> m_ViewportRenderer;
        Limen::PerspectiveCameraController m_ViewportCameraController;
        // 最近一帧中央区域；用于判定光标是否在视口内。
        UIRect m_CenterRect{};
        bool m_IsViewportHovered = false;
        bool m_IsNavigatingViewport = false;

        // 引擎默认字体的图集；面板文字只借用，生命周期由本 Layer 管理。
        Limen::Scope<Limen::FontAtlas> m_FontAtlas;
        //endregion
    };
}
