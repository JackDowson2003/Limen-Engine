//
// Created by chenlong on 2026/8/8.
//
#pragma once
#include "Input.h"

namespace Limen
{
    class MacInput final : public Input
    {
    public:
        MacInput() = default;

        ~MacInput() override;

    protected:
        [[nodiscard]] bool IsKeyPressedImpl(KeyCode keyCode) const override;

        [[nodiscard]] std::pair<float,float> GetMousePosImpl() const override;

        [[nodiscard]] bool IsMouseButtonPressedImpl(MouseButton button) const override;

        [[nodiscard]] float GetMouseXImpl() const override;

        [[nodiscard]] float GetMouseYImpl() const override;
    };
}
