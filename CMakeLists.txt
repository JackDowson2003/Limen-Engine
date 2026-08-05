cmake_minimum_required(VERSION 4.0) #Cmake版本

project(LimenEngine
        VERSION 0.1.0
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON) #要求编译器必须支持 C++20
set(CMAKE_CXX_EXTENSIONS OFF) #关闭编译器自己的非标准 C++ 扩展。

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
#编译器的提示
function(limen_configure_target target_name)
    if(MSVC)
        target_compile_options(${target_name}
                PRIVATE
                /W4
                /permissive-
        )

        if(LIMEN_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE /WX)
        endif()

    elseif(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(${target_name}
                PRIVATE
                -Wall #基础警告
                -Wextra #开启一批常用警告
                -Wpedantic #增加参数未使用、条件可疑等额外警告
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wformat=2
                -Wundef
                -Wnon-virtual-dtor
                -Wold-style-cast
                -Woverloaded-virtual
                -Wimplicit-fallthrough
                -Wnull-dereference
        )

        if(LIMEN_WARNINGS_AS_ERRORS)
            target_compile_options(${target_name} PRIVATE -Werror)
        endif()

        if(LIMEN_ENABLE_SANITIZERS)
            target_compile_options(${target_name}
                    PRIVATE
                    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
                    $<$<CONFIG:Debug>:-fno-omit-frame-pointer>
            )

            target_link_options(${target_name}
                    PRIVATE
                    $<$<CONFIG:Debug>:-fsanitize=address,undefined>
            )
        endif()
    endif()
endfunction()

add_subdirectory(LimenEngine)
add_subdirectory(LimenSandBox)
