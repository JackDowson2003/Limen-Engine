//
// Created by chenlong on 2026/8/5.
//
#include "Limen/Application.h"

#include "Limen/Events/ApplicationEvent.h"
#include "Limen/Log.h"

#include <chrono>
#include <thread>

namespace Limen
{
    Application::Application() = default;

    Application::~Application() = default;

    void Application::Run()
    {
        WindowResizeEvent e(1280,720);
        if (e.IsInCategory(EventCategoryApplication))
        {
            LM_TRACE(e.ToString());
        }
        if (e.IsInCategory(EventCategoryInput))
        {
            LM_TRACE(e.ToString());
        }
        while (true)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(16));
        }
    }
}
