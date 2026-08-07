//
// Created by chenlong on 2026/8/5.
//

#pragma once
#include "Application.h"
#include "Log.h"
#if !defined(LIMEN_PLATFORM_WINDOWS) && \
    !defined(LIMEN_PLATFORM_MACOS) && \
    !defined(LIMEN_PLATFORM_LINUX)

#error "LimenEngine does not support this platform"

#else

int main(int argc, char **argv)
{
    Limen::Log::Init();
    LM_CORE_WARN("Initializing LIMEN");
    {
        std::string deviceName;
        #if defined(LIMEN_PLATFORM_WINDOWS)
            deviceName = "Windows";
        #elif defined(LIMEN_PLATFORM_LINUX)
            deviceName = "Linux";
        #elif defined(LIMEN_PLATFORM_MACOS)
            deviceName = "MacOS";
        #endif

        LM_INFO("Initializing Client : {}",deviceName);
    }
    auto *app = Limen::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
#endif
