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

add_executable(LimenAssetIDTests
        AssetIDTests.cpp
)

limen_configure_target(LimenAssetIDTests)

target_link_libraries(LimenAssetIDTests PRIVATE LimenEngine)

add_test(
        NAME LimenAssetID
        COMMAND $<TARGET_FILE:LimenAssetIDTests>
)

add_executable(LimenAssetRegistryTests
        AssetRegistryTests.cpp
)

limen_configure_target(LimenAssetRegistryTests)

target_link_libraries(LimenAssetRegistryTests PRIVATE LimenEngine)

add_test(
        NAME LimenAssetRegistry
        COMMAND $<TARGET_FILE:LimenAssetRegistryTests>
)

add_executable(LimenProjectConfigurationTests
        ProjectConfigurationTests.cpp
)

limen_configure_target(LimenProjectConfigurationTests)

target_link_libraries(LimenProjectConfigurationTests PRIVATE LimenEngine)

add_test(
        NAME LimenProjectConfiguration
        COMMAND $<TARGET_FILE:LimenProjectConfigurationTests>
)

add_executable(LimenEditorSessionTests
        EditorSessionTests.cpp
)

limen_configure_target(LimenEditorSessionTests)

target_include_directories(LimenEditorSessionTests PRIVATE
        "${CMAKE_SOURCE_DIR}/Limen-Editor/src"
)

target_link_libraries(LimenEditorSessionTests PRIVATE LimenEngine)

add_test(
        NAME LimenEditorSession
        COMMAND $<TARGET_FILE:LimenEditorSessionTests>
)

# 用无副作用的错误参数检查 Editor 启动分支；避免 CPU 测试构造 OpenGL 窗口。
add_test(
        NAME LimenEditorStartupCli
        COMMAND ${CMAKE_COMMAND}
                "-DLIMEN_EDITOR_EXECUTABLE=$<TARGET_FILE:LimenEditor>"
                -P "${CMAKE_CURRENT_SOURCE_DIR}/EditorStartupCliTests.cmake"
)
set_tests_properties(LimenEditorStartupCli PROPERTIES TIMEOUT 15)

# EditorUILayer 头文件必须独立包含，避免被 EditorApp 的 include 顺序掩盖依赖。
add_executable(LimenEditorUILayerHeaderTests
        EditorUILayerHeaderTests.cpp
)

limen_configure_target(LimenEditorUILayerHeaderTests)

target_include_directories(LimenEditorUILayerHeaderTests PRIVATE
        "${CMAKE_SOURCE_DIR}/Limen-Editor/include"
        "${CMAKE_SOURCE_DIR}/Limen-Editor/src"
)

target_link_libraries(LimenEditorUILayerHeaderTests PRIVATE LimenEngine)

add_test(
        NAME LimenEditorUILayerHeader
        COMMAND $<TARGET_FILE:LimenEditorUILayerHeaderTests>
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

add_executable(LimenLStringTests
        LStringTests.cpp
)

limen_configure_target(LimenLStringTests)

target_include_directories(LimenLStringTests
        PRIVATE
        "${CMAKE_SOURCE_DIR}/LimenEngine/include"
)

add_test(
        NAME LimenLString
        COMMAND $<TARGET_FILE:LimenLStringTests>
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

add_executable(LimenEditorUILayoutTests
        EditorUILayoutTests.cpp
        "${CMAKE_SOURCE_DIR}/Limen-Editor/src/UI/EditorUILayout.cpp"
)

limen_configure_target(LimenEditorUILayoutTests)

target_include_directories(LimenEditorUILayoutTests PRIVATE
        "${CMAKE_SOURCE_DIR}/Limen-Editor/include"
)

add_test(
        NAME LimenEditorUILayout
        COMMAND $<TARGET_FILE:LimenEditorUILayoutTests>
)

if (APPLE)
    # 独立的可执行文件目录使路径测试能安全验证缺失/存在两种资源状态。
    add_executable(LimenPlatformPathsTests
            PlatformPathsTests.cpp
    )

    limen_configure_target(LimenPlatformPathsTests)

    target_link_libraries(LimenPlatformPathsTests PRIVATE LimenEngine)

    set_target_properties(LimenPlatformPathsTests PROPERTIES
            RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/PlatformPaths"
    )

    add_test(
            NAME LimenPlatformPaths
            COMMAND $<TARGET_FILE:LimenPlatformPathsTests>
    )

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

    # ShaderLibrary 的逻辑加载以测试可执行文件为锚点；显式准备同级内置资源。
    # 使用常驻构建依赖，Shader 文件变更后单独重建测试目标也会同步资源。
    add_custom_target(LimenOpenGLRenderer2DTestAssets
            COMMAND ${CMAKE_COMMAND} -E copy_directory_if_different
            "${CMAKE_SOURCE_DIR}/LimenEngine/assets"
            "$<TARGET_FILE_DIR:LimenOpenGLRenderer2DPipelineTests>/assets"
            COMMENT "Syncing built-in assets for Renderer2D GPU test"
            VERBATIM
    )
    add_dependencies(LimenOpenGLRenderer2DPipelineTests
            LimenOpenGLRenderer2DTestAssets
    )

    # 测试时故意使用与可执行文件和资源都不同的工作目录，防止回退到 CWD 仍然通过。
    set(LIMEN_RENDERER2D_TEST_CWD
            "${CMAKE_CURRENT_BINARY_DIR}/Renderer2DUnrelatedCwd"
    )
    file(MAKE_DIRECTORY "${LIMEN_RENDERER2D_TEST_CWD}")

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

    # 编译正式 Editor 使用的渲染实现，避免测试另一份复制的 Shader 或 UV 逻辑。
    add_executable(LimenOpenGLEditorUITextureTests
            OpenGLEditorUITextureTests.cpp
            "${CMAKE_SOURCE_DIR}/Limen-Editor/src/Render/EditorUIRenderer.cpp"
            "${CMAKE_SOURCE_DIR}/Limen-Editor/src/Render/OpenGL/OpenGLEditorUIRenderBackend.cpp"
    )

    limen_configure_target(LimenOpenGLEditorUITextureTests)

    target_include_directories(LimenOpenGLEditorUITextureTests PRIVATE
            "${CMAKE_SOURCE_DIR}/Limen-Editor/include"
            "${CMAKE_SOURCE_DIR}/Limen-Editor/src"
    )

    target_compile_definitions(LimenOpenGLEditorUITextureTests PRIVATE
            LIMEN_EDITOR_WITH_OPENGL
    )

    target_link_libraries(LimenOpenGLEditorUITextureTests PRIVATE
            LimenEngine
            glfw
            glad
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
                WORKING_DIRECTORY "${LIMEN_RENDERER2D_TEST_CWD}"
        )

        add_test(
                NAME LimenOpenGLModelImport
                COMMAND $<TARGET_FILE:LimenOpenGLModelImportTests>
        )
        set_tests_properties(LimenOpenGLModelImport PROPERTIES LABELS "gpu")

        add_test(
                NAME LimenOpenGLEditorUITexture
                COMMAND $<TARGET_FILE:LimenOpenGLEditorUITextureTests>
        )
        set_tests_properties(LimenOpenGLEditorUITexture PROPERTIES LABELS "gpu")
    endif ()
endif ()
