//
// Created by chenlong on 2026/8/7.
//
#pragma once

#include "Events/Event.h"

namespace Limen
{
    struct LIMEN_API WindowProps
    {
        std::string Title;
        uint32_t Width;
        uint32_t Height;

        WindowProps(std::string  title = "Limen Engine",
                    const uint32_t width = 1600,
                    const uint32_t height = 900)
            : Title(std::move(title)), Width(width), Height(height)
        {
        }
    };

    // Interface representing a desktop system based Window
    class LIMEN_API Window
    {
    public:
        using EventCallbackFn = std::function<void(Event& e)>;

        virtual ~Window() = default;

        virtual void OnUpdate() = 0;

        [[nodiscard]] virtual uint32_t GetWidth() const = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;

        // Window attributes
        virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        virtual void SetVSync(bool enabled) = 0;
        [[nodiscard]] virtual bool IsVSync() const = 0;

        [[nodiscard]] virtual void* GetNativeWindow() const = 0;

        static Window* Create(const WindowProps& props = WindowProps());
    };

}