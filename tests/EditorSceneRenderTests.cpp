#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <variant>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Asset/AssetManager.h"
#include "Limen/Core/Log.h"
#include "Limen/Project/ProjectConfiguration.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/Renderer/PerspectiveCamera.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneFile.h"
#include "Limen/Scene/SceneRenderer.h"
#include "Limen/Scene/SceneRuntime.h"
#include "Project/EditorSession.h"
#include "Render/EditorAssetSpawner.h"
#include "Render/EditorPrimitiveSpawner.h"
#include "Render/EditorSceneContent.h"
#include "Render/EditorSceneRenderParts.h"

namespace
{
    namespace fs = std::filesystem;

    bool Check(const char *name, const bool condition)
    {
        if (condition)
            return true;

        std::cerr << "FAIL: " << name << '\n';
        return false;
    }

    bool ReadPixel(
        const std::uintptr_t textureHandle,
        const int width,
        const int height,
        const int x,
        const int y,
        GLubyte output[4])
    {
        if (textureHandle == 0)
            return false;

        GLuint framebuffer = 0;
        glGenFramebuffers(1, &framebuffer);
        glBindFramebuffer(GL_FRAMEBUFFER, framebuffer);
        glFramebufferTexture2D(
            GL_FRAMEBUFFER,
            GL_COLOR_ATTACHMENT0,
            GL_TEXTURE_2D,
            static_cast<GLuint>(textureHandle),
            0);

        const bool complete =
            glCheckFramebufferStatus(GL_FRAMEBUFFER) ==
            GL_FRAMEBUFFER_COMPLETE;
        if (complete)
        {
            glReadBuffer(GL_COLOR_ATTACHMENT0);
            glReadPixels(
                std::clamp(x, 0, width - 1),
                std::clamp(y, 0, height - 1),
                1, 1, GL_RGBA, GL_UNSIGNED_BYTE, output);
        }

        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glDeleteFramebuffers(1, &framebuffer);
        return complete;
    }

    constexpr Limen::AssetID TwoMaterialAssetID{0x20, 0x21};

    bool TestOpenScene(
        LimenEditor::EditorSession &session,
        const LimenEditor::EditorSceneContent &content,
        const fs::path &projectRoot,
        const fs::path &modelPath)
    {
        constexpr Limen::SceneObjectID modelID{0x30, 0x31};
        constexpr Limen::SceneObjectID cubeID{0x30, 0x32};
        Limen::SceneDocument document;
        document.DisplayName = "Two materials and a cube";
        document.DirectionLight.Intensity = 0.0f;
        document.Objects.push_back({
            .ID = modelID,
            .Label = "Two material model",
            .Components = {{
                .ID = {0x40, 0x41},
                .Data = Limen::ModelRendererComponent{.ModelAssetID = TwoMaterialAssetID}
            }}
        });
        document.Objects.push_back({
            .ID = cubeID,
            .Label = "Restored cube",
            .Transform = {.Location = {2.0f, 0.0f, 0.0f}},
            .Components = {{
                .ID = {0x40, 0x42},
                .Data = Limen::PrimitiveRendererComponent{}
            }}
        });

        const fs::path scenePath = projectRoot / "valid.lscene";
        const fs::path missingModelScenePath = projectRoot / "missing-model.lscene";
        Limen::SceneFileError error;
        if (!Check("write valid Editor scene fixture",
                   Limen::TrySaveSceneFile(scenePath, document, error)))
            return false;
        document.DisplayName = "Missing model";
        if (!Check("write missing-model Editor scene fixture",
                   Limen::TrySaveSceneFile(missingModelScenePath, document, error)))
            return false;

        const Limen::SceneRenderPartsResolver resolveParts =
            [&](const Limen::SceneObjectComponent &component,
                Limen::LArray<Limen::SceneRenderObject> &parts,
                Limen::SceneFileError &componentError)
            {
                return LimenEditor::TryResolveEditorSceneRenderParts(
                    session.GetAssetRegistry(), content, component, parts, componentError);
            };

        if (!Check("Editor opens scene with model and Cube",
                   session.TryOpenScene(scenePath, resolveParts, error)))
        {
            std::cerr << error.Key.c_str() << ": " << error.Message.c_str() << '\n';
            return false;
        }

        bool passed = Check("open commits scene path and name, clears selection and dirty state",
                            session.GetScenePath() == scenePath &&
                            session.GetSceneDisplayName() == "Two materials and a cube" &&
                            session.GetSelectedObjectCount() == 0 && !session.IsDirty());
        Limen::LString label;
        passed &= Check("loaded model keeps its object ID and label",
                        session.TryGetObjectLabel(modelID, label) && label == "Two material model");
        passed &= Check("loaded Cube keeps its object ID and label",
                        session.TryGetObjectLabel(cubeID, label) && label == "Restored cube");

        int modelPartCount = 0;
        int cubePartCount = 0;
        session.GetScene().ForEachRenderableObject(
            [&](const Limen::SceneObjectHandle &, const Limen::SceneObjectID &id,
                const Limen::Ref<Limen::Mesh> &mesh, const Limen::Ref<Limen::Material> &material,
                const glm::mat4 &world)
            {
                if (id == modelID && mesh && material)
                    ++modelPartCount;
                if (id == cubeID && mesh == content.GetCubeMesh() && material == content.GetCubeMaterial() &&
                    std::abs(world[3].x - 2.0f) <= 1.0e-5f)
                    ++cubePartCount;
            });
        passed &= Check("one model object restores both draw parts and Cube uses built-in resources",
                        modelPartCount == 2 && cubePartCount == 1);

        Limen::PerspectiveCamera camera(45.0f, 1.0f, 0.1f, 100.0f);
        camera.SetPosition({0.0f, 0.0f, 4.0f});
        Limen::SceneRendererSpecification spec;
        spec.Width = 64;
        spec.Height = 64;
        spec.Samples = 1;
        spec.ShadowMapResolution = 64;
        spec.DebugName = "Editor Scene Open Test";
        Limen::SceneRenderer renderer(spec);
        Limen::Renderer::BeginFrame();
        renderer.Render(session.GetScene(), camera);
        Limen::Renderer::EndFrame();

        // 只开环境光，三角形颜色由各自 MTL 的 Ka 决定。避开对角线采样。
        GLubyte red[4]{};
        GLubyte blue[4]{};
        const auto color = renderer.GetFinalColorAttachmentHandle();
        const bool readable = ReadPixel(color, 64, 64, 37, 28, red) &&
                              ReadPixel(color, 64, 64, 27, 36, blue);
        passed &= Check("opened model renders both material colors",
                        readable && red[0] > red[2] + 8 && blue[2] > blue[0] + 8);

        const auto *openedScene = &session.GetScene();
        passed &= Check("active scene has an unsaved edit before failed open",
                        session.SelectObject(modelID) && session.SetObjectLabel(modelID, "Unsaved label") &&
                        session.IsDirty());
        // 第一次打开已经填充模型缓存。这里只删除临时项目复制出的源 OBJ。
        if (!Check("remove source model after it was cached", fs::remove(modelPath)))
            return false;
        const bool rejected = !session.TryOpenScene(missingModelScenePath, resolveParts, error);
        passed &= Check("cached model with missing source reports structured scene error",
                        rejected && error.Code == Limen::SceneFileErrorCode::FileAccess &&
                        error.File == missingModelScenePath && error.Key == "objects[0].components[0].model_asset_id" &&
                        !error.Message.empty());
        passed &= Check("failed open preserves active scene, edit state and path",
                        rejected && &session.GetScene() == openedScene && session.GetScenePath() == scenePath &&
                        session.GetSceneDisplayName() == "Two materials and a cube" &&
                        session.GetSelectedObjectID() == modelID && session.IsDirty() &&
                        session.TryGetObjectLabel(modelID, label) && label == "Unsaved label");
        return passed;
    }
}

int main()
{
    Limen::Log::Init();

    if (glfwInit() != GLFW_TRUE)
    {
        std::cerr << "GLFW initialization failed\n";
        return 1;
    }

    glfwDefaultWindowHints();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);
    glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

    GLFWwindow *window = glfwCreateWindow(
        64, 64, "Limen Editor renderable scene test", nullptr, nullptr);
    if (!window)
    {
        std::cerr << "OpenGL test window creation failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    if (gladLoadGL(glfwGetProcAddress) == 0)
    {
        std::cerr << "GLAD initialization failed\n";
        glfwDestroyWindow(window);
        glfwTerminate();
        return 1;
    }

    Limen::RendererAPI::SetAPI(Limen::RendererAPI::API::OPENGL);
    Limen::Renderer::Init();
    Limen::AssetManager::Init(LIMEN_TEST_ASSET_ROOT);

    bool success = true;
    {
        LimenEditor::EditorSceneContent content;
        success = Check("editor scene content initializes",
                        content.Initialize()) && success;
        success = Check("default cube mesh exists",
                        static_cast<bool>(content.GetCubeMesh())) && success;
        success = Check("default cube material exists",
                        static_cast<bool>(content.GetCubeMaterial())) && success;

        // Primitive Spawner 依赖真实 Mesh/Material，因此在 OpenGL 上下文中验证。
        if (content.GetCubeMesh() && content.GetCubeMaterial())
        {
            const auto stamp =
                std::chrono::steady_clock::now().time_since_epoch().count();
            const fs::path primitiveProjectRoot =
                fs::temp_directory_path() /
                ("limen-editor-primitive-render-test-" +
                 std::to_string(stamp));
            const fs::path primitiveAssetRoot =
                primitiveProjectRoot / "assets";
            fs::create_directories(primitiveAssetRoot);

            Limen::ProjectConfiguration primitiveConfiguration;
            primitiveConfiguration.AssetRootAbsolute = primitiveAssetRoot;
            auto primitiveRegistry =
                Limen::CreateScope<Limen::AssetRegistry>(
                    primitiveAssetRoot);
            Limen::AssetRegistryError primitiveRegistryError;
            const bool registryLoaded =
                primitiveRegistry->TryLoad(primitiveRegistryError);
            success = Check("primitive test registry loads",
                            registryLoaded) && success;

            if (registryLoaded)
            {
                LimenEditor::EditorSession primitiveSession(
                    std::move(primitiveConfiguration),
                    std::move(primitiveRegistry));
                LimenEditor::EditorPrimitiveSpawner primitiveSpawner;

                Limen::SceneTransform cubeTransform;
                cubeTransform.Location = {1.0f, 2.0f, 3.0f};
                Limen::LArray<Limen::SceneObjectID> cubes;
                std::string primitiveError;
                success = Check(
                    "cube primitive spawns",
                    primitiveSpawner.SpawnCube(
                        primitiveSession,
                        content,
                        cubeTransform,
                        cubes,
                        &primitiveError)) && success;
                success = Check("cube primitive creates one object",
                                cubes.getSize() == 1) && success;

                Limen::LString cubeLabel;
                success = Check(
                    "cube primitive receives default label",
                    cubes.getSize() == 1 &&
                    primitiveSession.TryGetObjectLabel(
                        cubes[0],
                        cubeLabel) &&
                    cubeLabel == "Cube") && success;

                Limen::SceneTransform storedCubeTransform;
                success = Check(
                    "cube primitive stores placement transform",
                    cubes.getSize() == 1 &&
                    primitiveSession.TryGetObjectTransform(
                        cubes[0],
                        storedCubeTransform) &&
                    std::abs(storedCubeTransform.Location.x - 1.0f) <=
                    1.0e-5f &&
                    std::abs(storedCubeTransform.Location.y - 2.0f) <=
                    1.0e-5f &&
                    std::abs(storedCubeTransform.Location.z - 3.0f) <=
                    1.0e-5f) && success;

                std::uint32_t renderVisits = 0;
                primitiveSession.GetScene().ForEachRenderableObject(
                    [&](const Limen::SceneObjectHandle &,
                        const Limen::SceneObjectID &visitedID,
                        const Limen::Ref<Limen::Mesh> &,
                        const Limen::Ref<Limen::Material> &,
                        const glm::mat4 &)
                    {
                        if (cubes.getSize() == 1 &&
                            visitedID == cubes[0])
                        {
                            ++renderVisits;
                        }
                    });
                success = Check(
                    "cube primitive attaches one render component",
                    renderVisits == 1) && success;

                Limen::SceneObjectHandle cubeHandle{};
                std::uint32_t authoringComponents = 0;
                bool primitiveIsCube = false;
                success = Check(
                    "spawned cube can be found",
                    cubes.getSize() == 1 &&
                    primitiveSession.GetScene().TryFindObject(
                        cubes[0],
                        cubeHandle)) && success;
                if (cubeHandle.IsValid())
                {
                    primitiveSession.GetScene().ForEachObjectComponent(
                        cubeHandle,
                        [&](const Limen::SceneObjectComponent &component)
                        {
                            ++authoringComponents;
                            const auto *primitive =
                                std::get_if<Limen::PrimitiveRendererComponent>(
                                    &component.Data);
                            primitiveIsCube =
                                primitive &&
                                primitive->Primitive ==
                                Limen::PrimitiveType::Cube;
                        });
                }
                success = Check(
                    "cube primitive attaches one authoring component",
                    authoringComponents == 1) && success;
                success = Check(
                    "cube primitive stores Cube type",
                    primitiveIsCube) && success;

                Limen::LArray<Limen::SceneObjectID> secondCube;
                Limen::SceneTransform secondCubeTransform;
                secondCubeTransform.Location = {-1.0f, 2.0f, 3.0f};
                success = Check(
                    "second cube primitive spawns",
                    primitiveSpawner.SpawnCube(
                        primitiveSession,
                        content,
                        secondCubeTransform,
                        secondCube)) && success;
                Limen::LString secondCubeLabel;
                success = Check(
                    "second cube primitive receives unique label",
                    secondCube.getSize() == 1 &&
                    primitiveSession.TryGetObjectLabel(
                        secondCube[0],
                        secondCubeLabel) &&
                    secondCubeLabel == "Cube_2") && success;
            }

            fs::remove_all(primitiveProjectRoot);
        }

        Limen::Scene scene;
        if (content.GetCubeMesh() && content.GetCubeMaterial())
        {
            Limen::SceneRenderObject object;
            object.MeshResource = content.GetCubeMesh();
            object.MaterialResource = content.GetCubeMaterial();
            object.Transform = glm::translate(
                glm::mat4{1.0f}, glm::vec3{2.0f, 0.0f, 0.0f});

            Limen::SceneTransform objectTransform;
            objectTransform.Location = {2.0f, 0.0f, 0.0f};
            // 根绕 Y 轴旋转 180°，将部件的局部偏移旋回原点，保留原来的取样位置。
            objectTransform.Rotation = glm::quat{0.0f, 0.0f, 1.0f, 0.0f};
            const Limen::SceneObjectHandle objectHandle =
                scene.AddRenderableObject(object, objectTransform);
            success = Check("renderable scene object is created",
                            objectHandle.IsValid()) && success;

            Limen::SceneObjectID objectID{};
            Limen::SceneObjectID picked{};
            success = Check("pick uses root TRS times component-local matrix",
                            scene.TryGetObjectID(objectHandle, objectID) &&
                            scene.PickObject(
                                {0.0f, 0.0f, 4.0f}, {0.0f, 0.0f, -1.0f}, picked) &&
                            picked == objectID) && success;
        }
        else
        {
            std::cerr << "Skipping renderable scene check: content unavailable\n";
            success = false;
        }

        Limen::PerspectiveCamera camera(45.0f, 1.0f, 0.1f, 100.0f);
        camera.SetPosition({0.0f, 0.0f, 4.0f});

        Limen::SceneRendererSpecification specification;
        specification.Width = 64;
        specification.Height = 64;
        specification.Samples = 1;
        specification.ShadowMapResolution = 64;
        specification.ClearColor = {0.1f, 0.1f, 0.1f, 1.0f};
        specification.DebugName = "Editor Renderable Scene Test";

        {
            Limen::SceneRenderer renderer(specification);
            Limen::Renderer::BeginFrame();
            renderer.Render(scene, camera);
            Limen::Renderer::EndFrame();

            const std::uintptr_t finalColor =
                renderer.GetFinalColorAttachmentHandle();
            success = Check("scene renderer produces final color",
                            finalColor != 0) && success;

            GLubyte center[4]{};
            GLubyte corner[4]{};
            const bool centerRead =
                ReadPixel(finalColor, 64, 64, 32, 32, center);
            const bool cornerRead =
                ReadPixel(finalColor, 64, 64, 4, 4, corner);
            success = Check("final color is readable",
                            centerRead && cornerRead) && success;

            if (centerRead && cornerRead)
            {
                const int difference = std::max({
                    std::abs(static_cast<int>(center[0]) - corner[0]),
                    std::abs(static_cast<int>(center[1]) - corner[1]),
                    std::abs(static_cast<int>(center[2]) - corner[2])
                });
                success = Check("renderable cube changes center pixel",
                                difference > 4) && success;
            }
        }
    }

    // 资产驱动端到端：临时项目中的 OBJ → 选中资产 → Spawner → SceneRenderer。
    {
        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        const fs::path projectRoot =
            fs::temp_directory_path() /
            ("limen-editor-spawn-test-" + std::to_string(stamp));
        const fs::path assetRoot = projectRoot / "assets";
        const fs::path modelDirectory = assetRoot / "models";
        fs::create_directories(modelDirectory);

        const fs::path fixtureRoot = LIMEN_TEST_SANDBOX_ASSET_ROOT;
        fs::copy_file(
            fixtureRoot / "models/TestCube.obj",
            modelDirectory / "TestCube.obj",
            fs::copy_options::overwrite_existing);
        const fs::path twoMaterialModelPath = modelDirectory / "TestTwoMaterials.obj";
        fs::copy_file(fixtureRoot / "models/TestTwoMaterials.obj", twoMaterialModelPath);
        fs::copy_file(fixtureRoot / "models/TestTwoMaterials.mtl", modelDirectory / "TestTwoMaterials.mtl");
        fs::create_directory(assetRoot / "textures");
        fs::copy_file(fixtureRoot / "textures/checkerboard.png", assetRoot / "textures/checkerboard.png");
        fs::copy_file(fixtureRoot / "textures/test-tilted-normal.png", assetRoot / "textures/test-tilted-normal.png");

        {
            std::ofstream sidecar(modelDirectory / "TestCube.obj.lmmeta");
            sidecar << "schema_version = 1\n"
                    << "asset_id = \"00112233-4455-6677-8899-aabbccddeeff\"\n";
        }
        {
            std::ofstream sidecar(modelDirectory / "TestTwoMaterials.obj.lmmeta");
            sidecar << "schema_version = 1\n"
                    << "asset_id = \"" << Limen::AssetIDToString(TwoMaterialAssetID).c_str() << "\"\n";
        }

        // 重新指向临时项目，确保 AssetManager 从 Registry 的相对路径加载模型。
        Limen::AssetManager::Shutdown();
        Limen::AssetManager::Init(assetRoot);

        Limen::ProjectConfiguration configuration;
        configuration.AssetRootAbsolute = assetRoot;
        auto registry =
            Limen::CreateScope<Limen::AssetRegistry>(assetRoot);
        Limen::AssetRegistryError registryError;
        success = Check("spawn test registry loads",
                        registry->TryLoad(registryError)) && success;

        LimenEditor::EditorSession session(
            std::move(configuration),
            std::move(registry));

        const Limen::AssetID assetID{
            0x0011223344556677ULL,
            0x8899aabbccddeeffULL
        };
        success = Check("spawn asset can be selected",
                        session.SelectAsset(assetID)) && success;

        LimenEditor::EditorSceneContent spawnContent;
        success = Check("spawn content initializes",
                        spawnContent.Initialize()) && success;

        LimenEditor::EditorAssetSpawner spawner;
        Limen::LArray<Limen::SceneObjectID> created;
        Limen::SceneTransform spawnTransform;
        spawnTransform.Location = {0.5f, 0.0f, 0.0f};
        success = Check("selected model spawns scene objects",
                        spawner.SpawnSelectedAsset(
                            session, spawnContent, spawnTransform, created)) && success;
        success = Check("spawn creates exactly one scene object",
                        created.getSize() == 1) && success;
        success = Check("spawn selects created objects",
                        session.GetSelectedObjectCount() == 1 &&
                        session.GetSelectedObjectID() == created[0]) && success;

        Limen::LString spawnedLabel;
        success = Check("spawn applies model label",
                        created.getSize() == 1 &&
                        session.TryGetObjectLabel(
                            created[0],
                            spawnedLabel) &&
                        spawnedLabel == "TestCube") && success;

        Limen::SceneObjectHandle createdHandle{};
        int authoringComponents = 0;
        Limen::AssetID authoringModelID{};
        success = Check("spawned object can be found",
                        created.getSize() == 1 &&
                        session.GetScene().TryFindObject(
                            created[0],
                            createdHandle)) && success;
        if (createdHandle.IsValid())
        {
            session.GetScene().ForEachObjectComponent(
                createdHandle,
                [&](const Limen::SceneObjectComponent &component)
                {
                    ++authoringComponents;
                    if (Limen::GetSceneComponentKind(component) ==
                        Limen::SceneComponentKind::ModelRenderer)
                    {
                        authoringModelID =
                            std::get<Limen::ModelRendererComponent>(
                                component.Data).ModelAssetID;
                    }
                });
        }
        success = Check("spawn attaches one authoring component",
                        authoringComponents == 1) && success;
        success = Check("spawned authoring component stores model asset ID",
                        authoringModelID == assetID) && success;

        glm::mat4 spawnedTransform{1.0f};
        success = Check("spawn applies object transform",
                        created.getSize() == 1 &&
                        session.TryGetObjectTransform(
                            created[0],
                            spawnedTransform) &&
                        std::abs(spawnedTransform[3][0] - 0.5f) <= 1.0e-6f) &&
                  success;

        Limen::SceneTransform spawnedAuthoring;
        success = Check("spawn writes authoring transform",
                        created.getSize() == 1 &&
                        session.TryGetObjectTransform(
                            created[0],
                            spawnedAuthoring) &&
                        std::abs(spawnedAuthoring.Location.x - 0.5f) <=
                        1.0e-6f) &&
                  success;

        Limen::SceneObjectID picked{};
        success = Check("spawned model can be picked",
                        session.GetScene().PickObject(
                            {0.5f, 0.0f, 4.0f},
                            {0.0f, 0.0f, -1.0f},
                            picked) &&
                        picked == created[0]) && success;

        const Limen::SceneObjectID missSentinel{
            0xDEADBEEFDEADBEEFULL,
            0xDEADBEEFDEADBEEFULL
        };
        Limen::SceneObjectID missOutput = missSentinel;
        success = Check("missed pick preserves output",
                        !session.GetScene().PickObject(
                            {10.0f, 10.0f, 4.0f},
                            {0.0f, 0.0f, -1.0f},
                            missOutput) &&
                        missOutput == missSentinel) && success;

        Limen::PerspectiveCamera spawnCamera(45.0f, 1.0f, 0.1f, 100.0f);
        spawnCamera.SetPosition({0.5f, 0.0f, 4.0f});

        Limen::SceneRendererSpecification spawnSpec;
        spawnSpec.Width = 64;
        spawnSpec.Height = 64;
        spawnSpec.Samples = 1;
        spawnSpec.ShadowMapResolution = 64;
        spawnSpec.ClearColor = {0.1f, 0.1f, 0.1f, 1.0f};
        spawnSpec.DebugName = "Editor Asset Spawn Test";

        {
            Limen::SceneRenderer spawnRenderer(spawnSpec);
            Limen::Renderer::BeginFrame();
            spawnRenderer.Render(session.GetScene(), spawnCamera);
            Limen::Renderer::EndFrame();

            GLubyte center[4]{};
            GLubyte corner[4]{};
            const std::uintptr_t finalColor =
                spawnRenderer.GetFinalColorAttachmentHandle();
            const bool readable =
                finalColor != 0 &&
                ReadPixel(finalColor, 64, 64, 32, 32, center) &&
                ReadPixel(finalColor, 64, 64, 4, 4, corner);
            success = Check("spawned model renders readable pixels",
                            readable) && success;

            if (readable)
            {
                const int difference = std::max({
                    std::abs(static_cast<int>(center[0]) - corner[0]),
                    std::abs(static_cast<int>(center[1]) - corner[1]),
                    std::abs(static_cast<int>(center[2]) - corner[2])
                });
                success = Check("spawned model changes center pixel",
                                difference > 4) && success;
            }
        }

        Limen::LArray<Limen::SceneObjectID> secondSpawn;
        Limen::SceneTransform secondSpawnTransform;
        secondSpawnTransform.Location = {-0.5f, 0.0f, 0.0f};
        success = Check("second model instance spawns",
                        spawner.SpawnSelectedAsset(
                            session,
                            spawnContent,
                            secondSpawnTransform,
                            secondSpawn)) && success;
        Limen::LString secondLabel;
        success = Check("second model instance receives unique label",
                        secondSpawn.getSize() == 1 &&
                        session.TryGetObjectLabel(
                            secondSpawn[0],
                            secondLabel) &&
                        secondLabel == "TestCube_2") && success;

        success = TestOpenScene(session, spawnContent, projectRoot, twoMaterialModelPath) && success;
        fs::remove_all(projectRoot);
    }

    Limen::AssetManager::Shutdown();
    Limen::Renderer::Shutdown();

    const GLenum error = glGetError();
    if (error != GL_NO_ERROR)
    {
        std::cerr << "Editor scene render GL error: " << error << '\n';
        success = false;
    }

    glfwDestroyWindow(window);
    glfwTerminate();

    if (success)
        std::cout << "Editor renderable scene: PASS\n";
    return success ? 0 : 1;
}
