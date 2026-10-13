add_executable(LimenPostProcessReferenceTests
        PostProcessReferenceTests.cpp
)

limen_configure_target(LimenPostProcessReferenceTests)

add_test(
        NAME LimenPostProcessReference
        COMMAND $<TARGET_FILE:LimenPostProcessReferenceTests>
)

add_executable(LimenAxisAlignedBoundingBoxTests
        AxisAlignedBoundingBoxTests.cpp
)

limen_configure_target(LimenAxisAlignedBoundingBoxTests)

target_include_directories(LimenAxisAlignedBoundingBoxTests
        PRIVATE
        "${CMAKE_SOURCE_DIR}/LimenEngine/include"
)

target_link_libraries(LimenAxisAlignedBoundingBoxTests PRIVATE glm::glm)

add_test(
        NAME LimenAxisAlignedBoundingBox
        COMMAND $<TARGET_FILE:LimenAxisAlignedBoundingBoxTests>
)

add_executable(LimenAssetPathTests
        AssetPathTests.cpp
)

limen_configure_target(LimenAssetPathTests)

target_link_libraries(LimenAssetPathTests PRIVATE LimenEngine)

add_test(
        NAME LimenAssetPath
        COMMAND $<TARGET_FILE:LimenAssetPathTests>
)

add_executable(LimenLArrayTests
        LArrayTests.cpp
)

limen_configure_target(LimenLArrayTests)

target_link_libraries(LimenLArrayTests PRIVATE LimenEngine)

add_test(
        NAME LimenLArray
        COMMAND $<TARGET_FILE:LimenLArrayTests>
)

add_executable(LimenLStaticArrayTests
        LStaticArrayTests.cpp
)

limen_configure_target(LimenLStaticArrayTests)

target_include_directories(LimenLStaticArrayTests
        PRIVATE
        "${CMAKE_SOURCE_DIR}/LimenEngine/include"
)

add_test(
        NAME LimenLStaticArray
        COMMAND $<TARGET_FILE:LimenLStaticArrayTests>
)

add_executable(LimenLTupleTests
        LTupleTests.cpp
)

limen_configure_target(LimenLTupleTests)

target_include_directories(LimenLTupleTests
        PRIVATE
        "${CMAKE_SOURCE_DIR}/LimenEngine/include"
)

add_test(
        NAME LimenLTuple
        COMMAND $<TARGET_FILE:LimenLTupleTests>
)

add_executable(LimenLMapTests
        LMapTests.cpp
)

limen_configure_target(LimenLMapTests)

target_include_directories(LimenLMapTests
        PRIVATE
        "${CMAKE_SOURCE_DIR}/LimenEngine/include"
)

add_test(
        NAME LimenLMap
        COMMAND $<TARGET_FILE:LimenLMapTests>
)

add_executable(LimenLayerStackSceneRegressionTests
        LayerStackSceneRegressionTests.cpp
)

limen_configure_target(LimenLayerStackSceneRegressionTests)

target_link_libraries(LimenLayerStackSceneRegressionTests PRIVATE LimenEngine)

add_test(
        NAME LimenLayerStackSceneRegression
        COMMAND $<TARGET_FILE:LimenLayerStackSceneRegressionTests>
)

if (APPLE)
    option(
            LIMEN_ENABLE_OPENGL_GPU_TESTS
            "Register OpenGL tests in CTest (requires a macOS desktop session)"
            OFF
    )

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
            LIMEN_RENDERER2D_DISPLAY_VERT_PATH="${CMAKE_SOURCE_DIR}/LimenEngine/assets/shaders/OpenGL/Renderer2D/DisplayEncode.vert"
            LIMEN_RENDERER2D_DISPLAY_FRAG_PATH="${CMAKE_SOURCE_DIR}/LimenEngine/assets/shaders/OpenGL/Renderer2D/DisplayEncode.frag"
    )

    add_executable(LimenOpenGLRenderer2DPipelineTests
            OpenGLRenderer2DPipelineTests.cpp
    )

    limen_configure_target(LimenOpenGLRenderer2DPipelineTests)

    target_link_libraries(LimenOpenGLRenderer2DPipelineTests
            PRIVATE
            LimenEngine
            glfw
            glad
    )

    add_executable(LimenOpenGLModelImportTests
            OpenGLModelImportTests.cpp
    )

    limen_configure_target(LimenOpenGLModelImportTests)

    target_link_libraries(LimenOpenGLModelImportTests
            PRIVATE
            LimenEngine
            glfw
            glad
    )

    target_compile_definitions(LimenOpenGLModelImportTests
            PRIVATE
            LIMEN_TEST_TWO_MATERIAL_MODEL_PATH="${CMAKE_SOURCE_DIR}/LimenSandBox/assets/models/TestTwoMaterials.obj"
    )

    if (LIMEN_ENABLE_OPENGL_GPU_TESTS)
        add_test(
                NAME LimenOpenGLPostProcess
                COMMAND $<TARGET_FILE:LimenOpenGLPostProcessTests>
        )
        set_tests_properties(LimenOpenGLPostProcess PROPERTIES LABELS "gpu")

        add_test(
                NAME LimenOpenGLRenderer2DPipeline
                COMMAND $<TARGET_FILE:LimenOpenGLRenderer2DPipelineTests>
        )
        set_tests_properties(LimenOpenGLRenderer2DPipeline PROPERTIES
                LABELS "gpu"
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/LimenEngine"
        )

        add_test(
                NAME LimenOpenGLModelImport
                COMMAND $<TARGET_FILE:LimenOpenGLModelImportTests>
        )
        set_tests_properties(LimenOpenGLModelImport PROPERTIES LABELS "gpu")
    endif ()
endif ()
