//
// Created by chenlong on 2026/8/5.
//

#pragma once
#include "Application.h"
#if !defined(LM_PLATFORM_WINDOWS) && \
    !defined(LM_PLATFORM_MACOS) && \
    !defined(LM_PLATFORM_LINUX)

#error "LimenEngine does not support this platform"

#else
int main(int argc, char **argv)
{
    printf("Running Application\n");
    auto *app = Limen::CreateApplication();
    app->Run();
    delete app;
    return 0;
}
#endif
