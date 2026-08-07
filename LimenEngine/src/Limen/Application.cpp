//
// Created by chenlong on 2026/8/5.
//
#include "Application.h"

#include "Events/ApplicationEvent.h"
#include "Log.h"



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
