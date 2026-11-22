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

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Asset/AssetManager.h"
#include "Limen/Core/Log.h"
#include "Limen/Project/ProjectConfiguration.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/Renderer/PerspectiveCamera.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneRenderer.h"
#include "Project/EditorSession.h"
#include "Render/EditorAssetSpawner.h"
#include "Render/EditorSceneContent.h"

namespace
{
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

        Limen::Scene scene;
        if (content.GetCubeMesh() && content.GetCubeMaterial())
        {
            Limen::SceneRenderObject object;
            object.MeshResource = content.GetCubeMesh();
            object.MaterialResource = content.GetCubeMaterial();
            object.Transform = glm::mat4{1.0f};
            const Limen::SceneObjectHandle objectHandle =
                scene.AddRenderableObject(object);
            success = Check("renderable scene object is created",
                            objectHandle.IsValid()) && success;
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
        namespace fs = std::filesystem;

        const auto stamp =
            std::chrono::steady_clock::now().time_since_epoch().count();
        const fs::path projectRoot =
            fs::temp_directory_path() /
            ("limen-editor-spawn-test-" + std::to_string(stamp));
        const fs::path assetRoot = projectRoot / "assets";
        const fs::path modelDirectory = assetRoot / "models";
        fs::create_directories(modelDirectory);

        fs::copy_file(
            LIMEN_TEST_MODEL_PATH,
            modelDirectory / "TestCube.obj",
            fs::copy_options::overwrite_existing);

        {
            std::ofstream sidecar(modelDirectory / "TestCube.obj.lmmeta");
            sidecar << "schema_version = 1\n"
                    << "asset_id = \"00112233-4455-6677-8899-aabbccddeeff\"\n";
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
        const glm::mat4 spawnTransform = glm::translate(
            glm::mat4{1.0f},
            glm::vec3{0.5f, 0.0f, 0.0f});
        success = Check("selected model spawns scene objects",
                        spawner.SpawnSelectedAsset(
                            session, spawnContent, spawnTransform, created)) && success;
        success = Check("spawn creates exactly one scene object",
                        created.getSize() == 1) && success;
        success = Check("spawn selects created objects",
                        session.GetSelectedObjectCount() == 1 &&
                        session.GetSelectedObjectID() == created[0]) && success;

        glm::mat4 spawnedTransform{1.0f};
        success = Check("spawn applies object transform",
                        created.getSize() == 1 &&
                        session.TryGetObjectTransform(
                            created[0],
                            spawnedTransform) &&
                        std::abs(spawnedTransform[3][0] - 0.5f) <= 1.0e-6f) &&
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
