#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <iostream>

#include "Limen/Asset/AssetManager.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/Renderer/PerspectiveCamera.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneRenderer.h"
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
