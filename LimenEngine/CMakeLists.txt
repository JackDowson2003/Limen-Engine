# 平台无关的引擎实现。CONFIGURE_DEPENDS 让新增匹配文件时触发 CMake 重配置。
file(GLOB_RECURSE LIMEN_ENGINE_COMMON_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Core/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Application/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Renderer/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/RHI/Common/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Editor/ImGui/*.cpp"
)

set(LIMEN_ENGINE_SOURCES ${LIMEN_ENGINE_COMMON_SOURCES})

if (APPLE)
    # 当前已落地组合：macOS 窗口、GLFW 输入与 OpenGL RHI 后端。
    file(GLOB_RECURSE LIMEN_ENGINE_MACOS_OPENGL_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/GLFW/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Platform/macOS/*.cpp"
            "${CMAKE_CURRENT_SOURCE_DIR}/src/RHI/macOS/OpenGL/*.cpp"
    )
    list(APPEND LIMEN_ENGINE_SOURCES ${LIMEN_ENGINE_MACOS_OPENGL_SOURCES})
elseif (WIN32)
    message(FATAL_ERROR
            "The Windows D3D11/D3D12 backend has not been implemented yet"
    )
elseif (UNIX)
    message(FATAL_ERROR
            "The Linux OpenGL/Vulkan backend has not been implemented yet"
    )
endif ()

file(GLOB_RECURSE LIMEN_ENGINE_HEADERS
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.hpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.h"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# 由根目录选项决定引擎产物是静态库还是共享库。
if (LIMEN_ENGINE_SHARED)
    set(LIMEN_ENGINE_LIBRARY_TYPE SHARED)
else ()
    set(LIMEN_ENGINE_LIBRARY_TYPE STATIC)
endif ()


add_library(LimenEngine
        ${LIMEN_ENGINE_LIBRARY_TYPE}
        ${LIMEN_ENGINE_SOURCES}
        ${LIMEN_ENGINE_HEADERS}
)

# PCH 只加速引擎自身编译，不传播给 Sandbox，也不能替代公共头文件的自包含。
target_precompile_headers(LimenEngine
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Limen/lmpch.h
)

# GLAD 提供 OpenGL 函数指针加载代码。
add_library(glad STATIC
        vendor/glad/src/gl.c
)

target_include_directories(glad
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include"
)

# 关闭 GLFW 自带的文档、示例、测试与安装规则，只构建引擎需要的库。
set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)

add_subdirectory(vendor/glfw EXCLUDE_FROM_ALL)

# GLM 提供头文件形式的向量与矩阵运算目标 glm::glm。
add_subdirectory(vendor/glm)

# ImGui 核心与 GLFW/OpenGL3 后端共同组成引擎编辑器 UI 库。
add_library(imgui STATIC
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp

        vendor/imgui/backends/imgui_impl_glfw.cpp
        vendor/imgui/backends/imgui_impl_opengl3.cpp

        # vendor/imgui/imgui_demo.cpp  # 需要官方 Demo 时再启用。
)

# stb_image 的实现被单独编译，避免在多个翻译单元重复定义实现宏。
add_library(stb_image STATIC
        vendor/stb_image/stb_image.cpp
)

target_include_directories(imgui
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends
)

# stb_image 是第三方头文件；SYSTEM 可以抑制第三方实现产生的项目警告。
target_include_directories(stb_image
        SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# ImGui 后端直接调用 GLFW 与 OpenGL，因此由 imgui 目标私有链接二者。
target_link_libraries(imgui
        PRIVATE
        glfw
        glad
)

# 引擎实现直接使用窗口、OpenGL、编辑器 UI 和图像解码库。
target_link_libraries(LimenEngine
        PRIVATE
        glfw
        glad
)

target_link_libraries(LimenEngine
        PRIVATE
        imgui
)

target_link_libraries(LimenEngine
        PRIVATE
        stb_image
)

# GLM 类型出现在公共头文件中，因此使用 PUBLIC 传播给客户端目标。
target_link_libraries(LimenEngine PUBLIC glm::glm)

# 平台宏属于公共编译契约：客户端包含公共头文件时也需要看到相同宏。
if (WIN32)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_PLATFORM_WINDOWS
    )
elseif (APPLE)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_PLATFORM_MACOS
    )
elseif (UNIX)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_PLATFORM_LINUX
    )
else ()
    message(FATAL_ERROR "LimenEngine does not support this platform")
endif ()

target_include_directories(LimenEngine
        PUBLIC
        # 对外只暴露 include/ 下的稳定接口。
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        # src/ 中的平台与后端头文件只允许引擎实现包含。
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        # Log.h 暴露了 spdlog 类型，因此其头文件路径必须传播给客户端。
        SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include
)

# 共享库构建时，LM_BUILD_DLL 只供引擎本体导出符号；客户端只接收类型宏。
if (LIMEN_ENGINE_SHARED)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_ENGINE_SHARED
            PRIVATE
            LM_BUILD_DLL
    )
else ()
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_ENGINE_STATIC
    )
endif ()

target_compile_definitions(LimenEngine
        PUBLIC
        $<$<CONFIG:Debug>:LM_ENABLE_ASSERTS>
)

# 应用根目录定义的统一警告与 Sanitizer 策略。
limen_configure_target(LimenEngine)
