#add_library(LimenEngine STATIC
#        src/LimenEngine.cpp
#
#        include/LimenEngine.h
#) #创建名称为LimenEngine的静态库

file(GLOB_RECURSE LIMEN_ENGINE_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
)

file(GLOB_RECURSE LIMEN_ENGINE_HEADERS
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/include/*.h"
)


#add_library(LimenEngine STATIC
#        ${LIMEN_ENGINE_SOURCES}
#        ${LIMEN_ENGINE_HEADERS}
#)
if(LIMEN_ENGINE_SHARED)
    set(LIMEN_ENGINE_LIBRARY_TYPE SHARED)
else()
    set(LIMEN_ENGINE_LIBRARY_TYPE STATIC)
endif()



add_library(LimenEngine
        ${LIMEN_ENGINE_LIBRARY_TYPE}
        ${LIMEN_ENGINE_SOURCES}
        ${LIMEN_ENGINE_HEADERS}
)

# lmpch.h 是仅供 LimenEngine 编译使用的预编译头，不是公开 API。
target_precompile_headers(LimenEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Limen/lmpch.h
)

add_library(glad STATIC
        vendor/glad/src/gl.c
)

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



target_link_libraries(LimenEngine
        PRIVATE
        glfw
        glad
)



if(WIN32)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_PLATFORM_WINDOWS
    )
elseif(APPLE)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_PLATFORM_MACOS
    )
elseif(UNIX)
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_PLATFORM_LINUX
    )
else()
    message(FATAL_ERROR "LimenEngine does not support this platform")
endif()
target_include_directories(LimenEngine
        PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/include
        ${CMAKE_CURRENT_SOURCE_DIR}/include/Limen
        # spdlog 是随项目提交的 header-only 第三方库。Log.h 暴露了 spdlog 类型，
        # 因此使用 LimenEngine 的目标也需要能够找到它的头文件。
        SYSTEM PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/vendor/spdlog/include
) # 告诉编译器去这些目录查找头文件。

if(LIMEN_ENGINE_SHARED)
    target_compile_definitions(LimenEngine
            PUBLIC
                LIMEN_ENGINE_SHARED
            PRIVATE
                LM_BUILD_DLL #这个是为了让自己才可以看到 也就是修改掉LIMEN_API
    )
else()
    target_compile_definitions(LimenEngine
            PUBLIC
            LIMEN_ENGINE_STATIC
    )
endif()
target_compile_definitions(LimenEngine
        PUBLIC
        $<$<CONFIG:Debug>:LM_ENABLE_ASSERTS>
)
limen_configure_target(LimenEngine)
