//
// Created by chenlong on 2026/8/7.
//
#pragma once

#include <functional>
#include <string>
#include <utility>

#include "Limen/Events/Event.h"

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

        /**
         * @brief 轮询操作系统窗口和输入事件。
         *
         * 每帧开始时调用，使本帧Update能够使用最新输入状态。
         * 检测有没有输入
         */
        virtual void PollEvents() = 0;

        /**
         * @brief 将本帧渲染结果提交到窗口。
         *
         * OpenGL后端对应SwapBuffers；Metal对应present drawable；
         * Direct3D对应SwapChain::Present。
         */
        virtual void Present() = 0;

        [[nodiscard]] virtual uint32_t GetWidth() const = 0;
        [[nodiscard]] virtual uint32_t GetHeight() const = 0;

        // Window attributes
        inline virtual void SetEventCallback(const EventCallbackFn& callback) = 0;
        inline virtual void SetVSync(bool enabled) = 0;
        [[nodiscard]] inline virtual bool IsVSync() const = 0;

        [[nodiscard]] inline virtual void* GetNativeWindow() const = 0;

        static Window* Create(const WindowProps& props = WindowProps());
    };

}
