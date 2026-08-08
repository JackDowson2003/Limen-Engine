//
// Created by chenlong on 2026/8/8.
//

#pragma once
#include "Layer.h"
#include "Events/ApplicationEvent.h"
#include "Events/KeyEvent.h"
#include "Events/MouseEvent.h"

namespace Limen
{
    class LIMEN_API ImGUILayer : public Layer
    {
    public:
        ImGUILayer();

        ~ImGUILayer() override;

        virtual void OnAttach() override;

        virtual void OnDetach() override;

        virtual void OnEvent(Event &event) override;

        virtual void OnUpdate() override;

        virtual bool OnMouseButtonPressedEvent(const MouseButtonPressedEvent &event);

        virtual bool OnMouseButtonReleaseEvent(const MouseButtonReleasedEvent &event);

        virtual bool OnMouseButtonScrolledEvent(const MouseScrolledEvent &event);

        virtual bool OnMouseButtonMovedEvent(const MouseMovedEvent &event);

        virtual bool OnKeyPressedEvent(const KeyPressedEvent &event);

        virtual bool OnKeyReleasedEvent(const KeyReleasedEvent &event);

        virtual bool OnKeyTyped(const KeyTypedEvent &event);

        virtual bool OnWindowResizeEvent(const WindowResizeEvent &event);
    private:
        float m_Time{};
    };
}
