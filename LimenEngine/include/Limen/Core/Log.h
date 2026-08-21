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

        inline static Ref<spdlog::logger> &GetCoreLogger() { return s_CoreLogger; }
        inline static Ref<spdlog::logger> &GetClientLogger() { return s_ClientLogger; }

    private:
        //分两个是为了区分业务 inline可以让我们在.cpp中不用再去定义变量
        //这行同时是声明和定义。编译器会为它提供实际的存储空间，因此不需要在 Log.cpp 再定义。
        //不加它能清楚地区分公共接口和实现，也能减少头文件承担的初始化与析构工作
        // inline static Ref<spdlog::logger> s_CoreLogger;
        // inline static Ref<spdlog::logger> s_ClientLogger;
        static Ref<spdlog::logger> s_CoreLogger; //给出变量的声明
        static Ref<spdlog::logger> s_ClientLogger;
    };
}

//Core log macros
#define LM_CORE_TRACE(...) ::Limen::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define LM_CORE_INFO(...) ::Limen::Log::GetCoreLogger()->info(__VA_ARGS__)
#define LM_CORE_WARN(...) ::Limen::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define LM_CORE_ERROR(...) ::Limen::Log::GetCoreLogger()->error(__VA_ARGS__)
#define LM_CORE_FATAL(...) ::Limen::Log::GetCoreLogger()->critical(__VA_ARGS__)


//Client log macros
#define LM_TRACE(...) ::Limen::Log::GetClientLogger()->trace(__VA_ARGS__)
#define LM_INFO(...) ::Limen::Log::GetClientLogger()->info(__VA_ARGS__)
#define LM_WARN(...) ::Limen::Log::GetClientLogger()->warn(__VA_ARGS__)
#define LM_ERROR(...) ::Limen::Log::GetClientLogger()->error(__VA_ARGS__)
#define LM_FATAL(...) ::Limen::Log::GetClientLogger()->critical(__VA_ARGS__)
