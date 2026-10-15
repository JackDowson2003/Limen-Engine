//
// Created by chenlong on 2026/8/6.
//

#pragma once
#include "spdlog/spdlog.h"

#include "Limen/Core/Core.h"

namespace Limen
{
    class LIMEN_API Log
    {
    public:
        static void Init();

        static Ref<spdlog::logger> &GetCoreLogger() { return s_CoreLogger; }
        static Ref<spdlog::logger> &GetClientLogger() { return s_ClientLogger; }

    private:
        // 引擎日志与客户端日志使用不同名称，便于过滤输出来源。
        // 这里只声明静态成员，实际定义位于 Log.cpp。
        static Ref<spdlog::logger> s_CoreLogger;
        static Ref<spdlog::logger> s_ClientLogger;
    };
}
#ifdef LM_ENABLE_ASSERTS
// 引擎核心日志宏。
#define LM_CORE_TRACE(...) ::Limen::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LM_CORE_INFO(...) ::Limen::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LM_CORE_WARN(...) ::Limen::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LM_CORE_ERROR(...) ::Limen::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LM_CORE_FATAL(...) ::Limen::Log::GetCoreLogger()->critical(__VA_ARGS__)


// 客户端日志宏。
#define LM_TRACE(...) ::Limen::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LM_INFO(...) ::Limen::Log::GetClientLogger()->info(__VA_ARGS__)
#define LM_WARN(...) ::Limen::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LM_ERROR(...) ::Limen::Log::GetClientLogger()->error(__VA_ARGS__)
#define LM_FATAL(...) ::Limen::Log::GetClientLogger()->critical(__VA_ARGS__)
#else
// 引擎核心日志宏。
#define LM_CORE_TRACE(...) ((void)0)
#define LM_CORE_INFO(...) ((void)0)
#define LM_CORE_WARN(...) ((void)0)
#define LM_CORE_ERROR(...) ((void)0)
#define LM_CORE_FATAL(...) ((void)0)


// 客户端日志宏。
#define LM_TRACE(...) ((void)0)
#define LM_INFO(...) ((void)0)
#define LM_WARN(...) ((void)0)
#define LM_ERROR(...) ((void)0)
#define LM_FATAL(...) ((void)0)
#endif