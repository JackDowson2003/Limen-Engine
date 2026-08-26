add_executable(LimenSandBox
        src/SandBoxApp.cpp
        src/Example3DLayer.cpp
        src/SandBox2D.cpp
)

# Sandbox 使用引擎公共 API，并直接编写 ImGui 面板，因此显式链接两个目标。
target_link_libraries(LimenSandBox
        PRIVATE
            LimenEngine
            imgui
)

# 与引擎使用同一套警告和 Sanitizer 配置。
limen_configure_target(LimenSandBox)

# 从 LimenSandBox/ 回到仓库根目录。
get_filename_component(LIMEN_PROJECT_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/.."
        ABSOLUTE
)

# 开发期把引擎内置资源和 Sandbox 项目资源复制到可执行文件旁。
# 先复制引擎资源，再复制项目资源。
add_custom_command(
        TARGET LimenSandBox
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${LIMEN_PROJECT_ROOT}/LimenEngine/assets"
        "$<TARGET_FILE_DIR:LimenSandBox>/assets"

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:LimenSandBox>/assets"

        COMMENT "Copying LimenEngine and Sandbox assets"
)

# 将不同构建目录生成的可执行文件统一放到仓库根目录的 out/。
set_target_properties(LimenSandBox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${LIMEN_PROJECT_ROOT}/out"
)
