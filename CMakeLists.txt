cmake_minimum_required(VERSION 4.0)

project(LimenEngine
        VERSION 0.1.0
        LANGUAGES C CXX
)

# 统一 MSVC 运行时：Debug 使用 /MDd，其余配置使用 /MD。
if (MSVC)
    set(CMAKE_MSVC_RUNTIME_LIBRARY
            "MultiThreadedDLL$<$<CONFIG:Debug>:Debug>"
    )
endif ()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 可选的严格构建模式；默认关闭，避免第三方或过渡期警告阻断开发。
option(
        LIMEN_WARNINGS_AS_ERRORS
        "Treat compiler warnings as errors"
        OFF
)

option(
        LIMEN_ENABLE_SANITIZERS
        "Enable runtime sanitizers in Debug builds"
        OFF
)

option(
        LIMEN_ENGINE_SHARED
        "Build LimenEngine as a shared library"
        OFF
)

# 为引擎与 Sandbox 目标应用一致的警告和 Sanitizer 配置。
function(limen_configure_target target_name)
    if (MSVC)
        target_compile_options(${target_name}
                PRIVATE
                /W4
                /permissive-
        )

        if (LIMEN_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif ()

    elseif (CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name}
                PRIVATE
                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wformat=2
                -Wundef
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Werror=return-type
                -Woverloaded-virtual
                -Wimplicit-fallthrough
                -Wnull-dereference
        )

        if (LIMEN_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif ()

        # 只在 Debug 配置注入 AddressSanitizer 与 UndefinedBehaviorSanitizer。
        if (LIMEN_ENABLE_SANITIZERS)
            target_compile_options(${target_name}
                    PRIVATE
                    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
                    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
            )

            target_link_options(${target_name}
                    PRIVATE
                    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            )
        endif ()
    endif ()
endfunction ()

# 子目录分别定义 LimenEngine 库与 LimenSandBox 可执行文件。
add_subdirectory(LimenEngine)
add_subdirectory(LimenSandBox)
