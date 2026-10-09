add_executable(LimenPostProcessReferenceTests
        PostProcessReferenceTests.cpp
)

limen_configure_target(LimenPostProcessReferenceTests)

add_test(
        NAME LimenPostProcessReference
        COMMAND $<TARGET_FILE:LimenPostProcessReferenceTests>
)
if (APPLE)
    add_executable(LimenOpenGLPostProcessTests
            OpenGLPostProcessTests.cpp
    )

    limen_configure_target(LimenOpenGLPostProcessTests)

    target_link_libraries(LimenOpenGLPostProcessTests
            PRIVATE
            glfw
            glad
    )

    target_compile_definitions(LimenOpenGLPostProcessTests
            PRIVATE
            LIMEN_POSTPROCESS_VERT_PATH="${CMAKE_SOURCE_DIR}/LimenEngine/assets/shaders/OpenGL/Renderer3D/PostProcess.vert"
            LIMEN_POSTPROCESS_FRAG_PATH="${CMAKE_SOURCE_DIR}/LimenEngine/assets/shaders/OpenGL/Renderer3D/PostProcess.frag"
    )
endif ()