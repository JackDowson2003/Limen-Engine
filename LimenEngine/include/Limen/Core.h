//
// Created by chenlong on 2026/8/5.
//
#pragma once

#if defined(LIMEN_ENGINE_SHARED) && defined(LIMEN_ENGINE_STATIC)
    #error "LIMEN_ENGINE_SHARED and LIMEN_ENGINE_STATIC cannot both be defined"
#endif
#if defined(LIMEN_ENGINE_SHARED)
    #if defined(LIMEN_PLATFORM_WINDOWS)
        #ifdef LM_BUILD_DLL
            #define LIMEN_API __declspec(dllexport)
        #else
            #define LIMEN_API __declspec(dllimport)
        #endif
    #elif defined(LIMEN_PLATFORM_MACOS) || defined(LIMEN_PLATFORM_LINUX)
        #define LIMEN_API __attribute__((visibility("default")))
    #else
        #error "Platform not supported linux and other platform"
    #endif
#elif defined(LIMEN_ENGINE_STATIC)
    #define LIMEN_API
#else
    #error "Error!!! Unknown Type for platform!"
#endif

#define BIT(x) (1 << x)
