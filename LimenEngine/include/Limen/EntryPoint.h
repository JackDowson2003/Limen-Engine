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
    LM_INFO("Initializing var={0}",5);

    auto* app = Limen::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
#endif
