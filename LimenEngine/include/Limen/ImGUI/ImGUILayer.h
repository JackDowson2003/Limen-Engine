//
// Created by chenlong on 2026/8/8.
//

#pragma once
#include "../Layer.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

namespace Limen
{
    /**
    *  ImGUILayer
    ├─ OnAttach()：创建 ImGui Context、初始化 GLFW/OpenGL backend
    ├─ OnEvent()：把引擎事件交给 ImGui
    ├─ Begin()：NewFrame + 创建 DockSpace
    ├─ End()：Render + 提交 OpenGL 绘制
    └─ OnDetach()：销毁 ImGui backend / Context
     */
    class LIMEN_API ImGUILayer : public Layer
    {
    public:
        ImGUILayer();

        ~ImGUILayer() override;

        virtual void OnAttach() override;

        virtual void OnDetach() override;

        virtual void OnEvent(Event &event) override;

        // Begin() 与 End() 包围同一帧中所有 Layer 的 OnImGuiRender() 调用。
         void Begin();

         void End();


        virtual bool OnMouseButtonPressedEvent(const MouseButtonPressedEvent &event);

        virtual bool OnMouseButtonReleaseEvent(const MouseButtonReleasedEvent &event);

        virtual bool OnMouseButtonScrolledEvent(const MouseScrolledEvent &event);

        virtual bool OnMouseButtonMovedEvent(const MouseMovedEvent &event);

        virtual bool OnKeyPressedEvent(const KeyPressedEvent &event);

        virtual bool OnKeyReleasedEvent(const KeyReleasedEvent &event);

        virtual bool OnKeyTyped(const KeyTypedEvent &event);

    };
}
