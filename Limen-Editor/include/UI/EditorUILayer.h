//
// Created by chenlong on 2026/10/14.
//
#pragma once
#include <algorithm>
#include <utility>

#include "Render/EditorUIRenderer.h"
#include "Limen/Application/Layer.h"
#include "Limen/Core/Log.h"
#include "Limen/Application/Window.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"
#include "Limen/Application/Application.h"
#include "Limen/Asset/AssetManager.h"
#include "UI/EditorUILayout.h"
#include "Project/EditorSession.h"

namespace LimenEditor
{
    // 编辑器专用 UI Layer：管理当前单窗口占位布局及右侧分隔条的交互状态。
    class EditorUILayer final : public Limen::Layer
    {
    public:
        EditorUILayer(Limen::Ref<EditorSession> session = {})
            : Layer("Limen Editor"), m_Session(std::move(session))
        {
        }

        void OnAttach() override
        {
            if (!m_UIRenderer.Initialize())
                LM_ERROR("Editor UI renderer initialization failed");
        }

        void OnDetach() override
        {
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
            if (layout.Center.Width >= 64.0f && layout.Center.Height >= 64.0f)
            {
                const auto& white = Limen::AssetManager::GetWhiteTexture();
                if (white)
                    m_UIRenderer.DrawTexturedRect(
                        window,
                        {layout.Center.X + 16.0f, layout.Center.Y + 16.0f},
                        {48.0f, 48.0f},
                        *white
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
            // dispatcher 只在本次事件调用期间存在，用于按事件类型调用对应回调。
            Limen::EventDispatcher dispatcher(event);

            // press 仅提供鼠标按钮；按下位置需从当前单窗口的 Input 查询。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>([this](const Limen::MouseButtonPressedEvent &press)
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
        //endregion
    };
}
