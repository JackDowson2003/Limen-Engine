#add_library(LimenEngine STATIC
#        src/LimenEngine.cpp
#
#        include/LimenEngine.h
#) #创建名称为LimenEngine的静态库

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
    # 当前已经落地的组合：macOS平台窗口 + GLFW输入 + OpenGL RHI后端。
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


#add_library(LimenEngine STATIC
#        ${LIMEN_ENGINE_SOURCES}
#        ${LIMEN_ENGINE_HEADERS}
#)
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

# lmpch.h 是仅供 LimenEngine 编译使用的预编译头，不是公开 API。
target_precompile_headers(LimenEngine
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Limen/lmpch.h
)

add_library(glad STATIC
        vendor/glad/src/gl.c
)

#glad
target_include_directories(glad
        PUBLIC
        "${CMAKE_CURRENT_SOURCE_DIR}/vendor/glad/include"
)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)
set(GLFW_LIBRARY_TYPE STATIC CACHE STRING "" FORCE)

add_subdirectory(vendor/glfw EXCLUDE_FROM_ALL)

add_subdirectory(vendor/glm)

#imgui
add_library (imgui STATIC
        vendor/imgui/imgui.cpp
        vendor/imgui/imgui_draw.cpp
        vendor/imgui/imgui_tables.cpp
        vendor/imgui/imgui_widgets.cpp

        vendor/imgui/backends/imgui_impl_glfw.cpp
        vendor/imgui/backends/imgui_impl_opengl3.cpp

        #vendor/imgui/imgui_demo.cpp #test
)

add_library(stb_image STATIC
    vendor/stb_image/stb_image.cpp
)

#imgui
target_include_directories(imgui
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui
        PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/imgui/backends
)

target_include_directories(stb_image
        SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor
)

# ImGui 的 GLFW/OpenGL3 后端直接依赖这两个库。 所以需要让imgui Link上glfw glad
target_link_libraries(imgui
        PRIVATE
        glfw
        glad
)

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

target_link_libraries(LimenEngine PUBLIC glm::glm)

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
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        PRIVATE
        # 平台适配器（例如 GLFWKeyCodes.h）只允许引擎自身源文件包含。
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        # spdlog 是随项目提交的 header-only 第三方库。Log.h 暴露了 spdlog 类型，
        # 因此使用 LimenEngine 的目标也需要能够找到它的头文件。
        SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include
) # 告诉编译器去这些目录查找头文件。

if (LIMEN_ENGINE_SHARED)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_ENGINE_SHARED
            PRIVATE
            LM_BUILD_DLL #这个是为了让自己才可以看到 也就是修改掉LIMEN_API
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

limen_configure_target(LimenEngine)
