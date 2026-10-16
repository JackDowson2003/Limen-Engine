//
// Created by chenlong on 2026/10/14.
//
#pragma once
#include <algorithm>

#include "Render/EditorUIRenderer.h"
#include "Limen/Application/Layer.h"
#include "Limen/Core/Log.h"
#include "Limen/Application/Window.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"
#include "Limen/Application/Application.h"

namespace LimenEditor
{
    // 编辑器专用 UI Layer：管理当前单窗口占位布局及右侧分隔条的交互状态。
    class EditorUILayer final : public Limen::Layer
    {
    public:
        EditorUILayer()
            : Layer("Limen Editor")
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
            // 清除上一帧命中区域；窗口最小化或右侧栏消失时不能继续命中旧位置。
            m_RightSplitterRect = {};
            // UI 使用左上角为原点的逻辑窗口坐标，不使用帧缓冲像素尺寸。
            // width 是当前窗口内容区的逻辑宽度，转为 float 供布局计算使用。
            const float width = static_cast<float>(window.GetWindowWidth());
            // height 是当前窗口内容区的逻辑高度，不是帧缓冲像素高度。
            const float height = static_cast<float>(window.GetWindowHeight());

            // 窗口最小化时无需渲染
            if (width <= 0.0f || height <= 0.0f)
                return;

            // topHeight 是实际顶部占位区高度，不超过当前窗口高度。
            const float topHeight = std::min(kTopPlaceholderHeight, height);
            m_UIRenderer.DrawSolidRect(
                window,
                {0.0f, 0.0f},
                {width, topHeight},
                {0.17f, 0.18f, 0.20f, 1.0f}
            );

            // kMiddlePreferredPercent 是中央工作区的目标宽度占比，并非固定像素宽度。
            constexpr float kMiddlePreferredPercent = 0.65f;
            // kMinMiddleWidth 是窗口有足够空间时希望保留的中央最小逻辑宽度。
            constexpr float kMinMiddleWidth = 240.0f;

            // middleWidth 是受窗口宽度限制后的实际中央宽度；不能超过窗口。
            const float middleWidth = std::min(
                width,
                std::max(kMinMiddleWidth, width * kMiddlePreferredPercent)
            );
            // sideWidth 是左、右两个侧栏各自的逻辑宽度，均分中央以外的空间。
            const float sideWidth = (width - middleWidth) * 0.5f;

            // bodyHeight 是顶部占位区下方的高度；三栏只在这块区域内绘制。
            if (const float bodyHeight = height - topHeight; bodyHeight > 0.0f)
            {
                m_UIRenderer.DrawSolidRect(
                    window,
                    {0.0f, topHeight},
                    {sideWidth, bodyHeight},
                    {0.13f, 0.14f, 0.16f, 1.0f}
                );
                m_UIRenderer.DrawSolidRect(
                    window,
                    {sideWidth, topHeight},
                    {middleWidth, bodyHeight},
                    {0.08f, 0.09f, 0.11f, 1.0f}
                );
                // rightX 是右侧栏的左边界，坐标原点仍为窗口左上角。
                const float rightX = sideWidth + middleWidth;

                // splitterHeight 是实际分隔条高度；正文不足 4 单位时随正文缩小。
                const float splitterHeight = std::min(kRightSplitterHeight, bodyHeight);
                // availableHeight 只分给右上、右下面板，不包含分隔条本身。
                const float availableHeight = bodyHeight - splitterHeight;
                // rightTopHeight 是可用高度乘以当前比例得到的实际右上高度。
                const float rightTopHeight = availableHeight * m_RightTopRatio;
                // rightBottomHeight 占据扣除右上面板后的全部剩余可用高度。
                const float rightBottomHeight = availableHeight - rightTopHeight;
                // splitterY 是分隔条上沿的逻辑 Y 坐标，紧接右上面板。
                const float splitterY = topHeight + rightTopHeight;
                // 保存本帧实际绘制边界，供之后的鼠标按下事件命中测试。
                m_RightSplitterRect =
                {
                    .X = rightX,
                    .Y = splitterY,
                    .Width = sideWidth,
                    .Height = splitterHeight
                };

                m_UIRenderer.DrawSolidRect(
                    window,
                    {rightX, topHeight},
                    {sideWidth, rightTopHeight},
                    {0.15f, 0.16f, 0.18f, 1.0f}
                );
                m_UIRenderer.DrawSolidRect(
                    window,
                    {m_RightSplitterRect.X, m_RightSplitterRect.Y},
                    {m_RightSplitterRect.Width, m_RightSplitterRect.Height},
                    {0.35f, 0.37f, 0.42f, 1.0f}
                );
                m_UIRenderer.DrawSolidRect(
                    window,
                    {rightX, splitterY + splitterHeight},
                    {sideWidth, rightBottomHeight},
                    {0.21f, 0.22f, 0.24f, 1.0f}
                );
            }
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
                    const float height = static_cast<float>(window.GetWindowHeight());

                    // topHeight 是实际顶部占位区高度，不超过当前窗口高度。
                    const float topHeight = std::min(kTopPlaceholderHeight, height);
                    // bodyHeight 是顶部占位区下方留给三栏的高度。
                    const float bodyHeight = height - topHeight;

                    // 鼠标松开位置到上方的距离
                    const float distanceTopOfMouse = mouseY - topHeight;

                    // 当前按正文高度把松开位置换算为比例。
                    m_RightTopRatio = std::clamp(distanceTopOfMouse/bodyHeight,0.00001f,1.0f);

                    m_IsDraggingRightSplitter = false;
                    LM_INFO("Editor right splitter released");
                    return true;
                });
        }

    private:
        // region struct definition
        // 一次绘制得到的逻辑窗口坐标边界；只保存数字，不拥有窗口或 GPU 资源。
        struct UIRect
        {
            // X 是矩形左边界的逻辑横坐标。
            float X = 0.0f;
            // Y 是矩形上边界的逻辑纵坐标。
            float Y = 0.0f;
            // Width 是矩形的逻辑宽度；零表示没有可命中的横向区域。
            float Width = 0.0f;
            // Height 是矩形的逻辑高度；零表示没有可命中的纵向区域。
            float Height = 0.0f;

            // x、y 是待检测的逻辑鼠标坐标；右边界和下边界不属于本矩形。
            [[nodiscard]]
            bool Contains(const float x, const float y) const
            {
                return Width > 0.0f && Height > 0.0f
                       && x >= X && x < X + Width
                       && y >= Y && y < Y + Height;
            }
        };

        //endregion
        //region attributes definitions
        // 本 Layer 按值拥有 UI 绘制器；OnAttach 初始化，OnDetach 在 Context 销毁前关闭。
        EditorUIRenderer m_UIRenderer;
        // 顶部占位区的期望高度，单位为逻辑窗口单位；实际高度受窗口限制。
        static constexpr float kTopPlaceholderHeight = 128.0f;
        // 右侧上下分隔条的期望高度，单位为逻辑窗口单位。
        static constexpr float kRightSplitterHeight = 4.0f;

        // 右上面板的绘制比例；绘制时乘以扣除分隔条后的 availableHeight。
        float m_RightTopRatio = 0.5f;
        // 最近一次 OnUIRender 实际绘制的分隔条命中区域；每次绘制前清空并重建。
        UIRect m_RightSplitterRect{};
        // 左键是否从分隔条开始按下，等待松开时更新比例；当前并非实时拖动。
        bool m_IsDraggingRightSplitter = false;
        //endregion
    };
}
