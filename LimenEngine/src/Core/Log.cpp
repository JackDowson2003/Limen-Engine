//
// Created by chenlong on 2026/8/6.
//
#include "Limen/Core/Log.h"
#include "spdlog/sinks/stdout_color_sinks.h"

namespace Limen
{
    // 静态日志器在此分配存储；头文件只声明公共访问接口。
    Ref<spdlog::logger> Log::s_CoreLogger;
    Ref<spdlog::logger> Log::s_ClientLogger;

    void Log::Init()
    {
        spdlog::set_pattern("%^[%Y-%m-%d] %n [%T] %v%$");
        s_CoreLogger = spdlog::stdout_color_mt("LIMEN");
        s_CoreLogger->set_level(spdlog::level::trace);

        s_ClientLogger = spdlog::stdout_color_mt("APP");
        s_ClientLogger->set_level(spdlog::level::trace);
    }
}
