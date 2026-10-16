# 公共 Editor 源码与平台后端分别收集，避免在 macOS 编译将来的 DX12 实现。
file(GLOB LIMEN_EDITOR_COMMON_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
        "${CMAKE_CURRENT_SOURCE_DIR}/src/Render/*.cpp"
)

file(GLOB_RECURSE LIMEN_EDITOR_UI_SOURCES
        CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_SOURCE_DIR}/src/UI/*.cpp"
)

set(LIMEN_EDITOR_SOURCES
        ${LIMEN_EDITOR_COMMON_SOURCES}
        ${LIMEN_EDITOR_UI_SOURCES}
)

if (APPLE)
    file(GLOB_RECURSE LIMEN_EDITOR_OPENGL_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Render/OpenGL/*.cpp"
    )
    list(APPEND LIMEN_EDITOR_SOURCES ${LIMEN_EDITOR_OPENGL_SOURCES})
    set(LIMEN_EDITOR_BACKEND_DEFINITION LIMEN_EDITOR_WITH_OPENGL)
elseif (WIN32)
    file(GLOB_RECURSE LIMEN_EDITOR_DX12_SOURCES
            CONFIGURE_DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/src/Render/DX12/*.cpp"
    )
    # 当前 DX12 实现不存在；只有加入实际源文件后才启用对应分支。
    if (LIMEN_EDITOR_DX12_SOURCES)
        list(APPEND LIMEN_EDITOR_SOURCES ${LIMEN_EDITOR_DX12_SOURCES})
        set(LIMEN_EDITOR_BACKEND_DEFINITION LIMEN_EDITOR_WITH_DX12)
    endif ()
endif ()

add_executable(LimenEditor ${LIMEN_EDITOR_SOURCES})

target_link_libraries(LimenEditor PRIVATE LimenEngine)
target_include_directories(LimenEditor PRIVATE
        "${CMAKE_CURRENT_SOURCE_DIR}/include"
        "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

if (LIMEN_EDITOR_BACKEND_DEFINITION)
    target_compile_definitions(LimenEditor PRIVATE ${LIMEN_EDITOR_BACKEND_DEFINITION})
endif ()

limen_configure_target(LimenEditor)

get_filename_component(LIMEN_PROJECT_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ABSOLUTE
)

set_target_properties(LimenEditor PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${LIMEN_PROJECT_ROOT}/out"
)

# Editor 单独构建时也要准备 Renderer2D 使用的引擎内置 Shader。
add_custom_target(LimenEditorAssets
        COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
        "${LIMEN_PROJECT_ROOT}/LimenEngine/assets"
        "$<TARGET_FILE_DIR:LimenEditor>/assets"
        COMMENT "Syncing LimenEngine assets for Editor"
        VERBATIM
)

add_dependencies(LimenEditor LimenEditorAssets)
