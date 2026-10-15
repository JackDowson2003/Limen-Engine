add_executable(LimenEditor
        src/EditorApp.cpp
)

target_link_libraries(LimenEditor PRIVATE LimenEngine)
target_include_directories(LimenEditor PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/include")

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