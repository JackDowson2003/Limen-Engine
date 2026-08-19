//
// Created by chenlong on 2026/8/5.
//
#pragma once
#include <memory>
#include <utility>

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
#error "Error!!! Unknown Compiled Type for platform!"
#endif


#if defined(LIMEN_PLATFORM_WINDOWS)
#define LM_DEBUGBREAK() __debugbreak()
#elif defined(LIMEN_PLATFORM_MACOS)
#if defined(__clang__)
#define LM_DEBUGBREAK() __builtin_debugtrap()
#else
#define LM_DEBUGBREAK() __builtin_trap()
#endif
#elif defined(LIMEN_PLATFORM_LINUX)
#define LM_DEBUGBREAK() __builtin_trap()
#endif


#ifdef LM_ENABLE_ASSERTS

#define LM_ASSERT(x, ...)                                      \
        do {                                                       \
            if (!(x)) {                                           \
                LM_ERROR("Assertion failed: ({})", #x);           \
                LM_ERROR(__VA_ARGS__);                            \
                LM_DEBUGBREAK();                                  \
            }                                                      \
        } while (false)

#define LM_CORE_ASSERT(x, ...)                                 \
        do {                                                       \
            if (!(x)) {                                           \
                LM_CORE_ERROR("Assertion failed: ({})", #x);      \
                LM_CORE_ERROR(__VA_ARGS__);                       \
                LM_DEBUGBREAK();                                  \
            }                                                      \
        } while (false)

#else
#define LM_ASSERT(x, ...) ((void)0)
#define LM_CORE_ASSERT(x, ...) ((void)0)
#endif

#define BIT(x) (1 << x)


namespace Limen
{
    template<typename T>
    using Scope = std::unique_ptr<T>;

    template<typename T>
    using Ref = std::shared_ptr<T>;

    template<typename T, typename... Args>
    [[nodiscard]] constexpr Scope<T> CreateScope(Args &&... args)
    {
        return std::make_unique<T>(
            std::forward<Args>(args)...
        );
    }

    template<typename T, typename... Args>
    [[nodiscard]] constexpr Ref<T> CreateRef(Args &&... args)
    {
        return std::make_shared<T>(
            std::forward<Args>(args)...
        );
    }
}
