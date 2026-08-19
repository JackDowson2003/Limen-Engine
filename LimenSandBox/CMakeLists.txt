add_executable(LimenSandBox
        src/SandBoxApp.cpp
)

target_link_libraries(LimenSandBox #把 Engine 静态库链接到 Sandbox。
        PRIVATE
            LimenEngine
            # Sandbox 自己绘制 ImGui 面板，因此显式依赖 ImGui。
            imgui
)

limen_configure_target(LimenSandBox)

add_custom_command(
        TARGET LimenSandBox
        POST_BUILD

        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${CMAKE_CURRENT_SOURCE_DIR}/assets"
        "$<TARGET_FILE_DIR:LimenSandBox>/assets"

        COMMENT "Copying Sandbox assets"
)


#它用来计算项目根目录。
get_filename_component(LIMEN_PROJECT_ROOT
        "${CMAKE_CURRENT_SOURCE_DIR}/.." #/Users/cl/Desktop/language_study/Limen-Engine
        ABSOLUTE
)

#设置 LimenSandBox 目标的属性。指定最终可执行文件的输出目录。
set_target_properties(LimenSandBox PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${LIMEN_PROJECT_ROOT}/out"
)
