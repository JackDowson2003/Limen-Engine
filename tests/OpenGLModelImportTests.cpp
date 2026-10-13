#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <glad/gl.h>

#include <cstdint>
#include <filesystem>
#include <iostream>
#include <string_view>

#include "Limen/Asset/ModelImporter.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/RHI/RendererAPI.h"
#include "Limen/RHI/VertexArray.h"

namespace
{
    bool Check(std::string_view name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    bool CheckBounds(const Limen::AxisAlignedBoundingBox &bounds)
    {
        return bounds.IsValid() &&
               bounds.Min.x == -0.5f && bounds.Min.y == -0.5f && bounds.Min.z == 0.0f &&
               bounds.Max.x == 0.5f && bounds.Max.y == 0.5f && bounds.Max.z == 0.0f;
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

    GLFWwindow *window = glfwCreateWindow(16, 16, "Limen OBJ import test", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cerr << "OpenGL test window creation failed\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    bool success = gladLoadGL(glfwGetProcAddress) != 0;
    if (!success)
    {
        std::cerr << "GLAD initialization failed\n";
    } else
    {
        GLint major = 0;
        GLint minor = 0;
        glGetIntegerv(GL_MAJOR_VERSION, &major);
        glGetIntegerv(GL_MINOR_VERSION, &minor);
        success = Check("OpenGL 4.1 context", major > 4 || (major == 4 && minor >= 1));
    }

    if (success)
    {
        Limen::RendererAPI::SetAPI(Limen::RendererAPI::API::OPENGL);

        // Import 会创建 Mesh 的 GPU 对象；model 必须先于当前 Context 销毁。
        {
            const std::filesystem::path sourcePath = LIMEN_TEST_TWO_MATERIAL_MODEL_PATH;
            auto model = Limen::ModelImporter::Import(sourcePath);
            success &= Check("two-material OBJ import", model != nullptr);

            if (model)
            {
                const auto &slots = model->GetMaterialSlots();
                const auto &parts = model->GetParts();
                success &= Check("material slot count after LArray migration", slots.getSize() == 2);
                success &= Check("model part count after LArray migration", parts.getSize() == 2);
                success &= Check("model bounds merge both triangles", CheckBounds(model->GetLocalBounds()));

                if (slots.getSize() == 2 && parts.getSize() == 2)
                {
                    success &= Check("material slot order", slots[0].Name == "Red" && slots[1].Name == "Blue");
                    success &= Check("model part order and material mapping",
                                     parts[0].Name == "TwoMaterialQuad/Red" && parts[0].MaterialSlot == 0 &&
                                     parts[1].Name == "TwoMaterialQuad/Blue" && parts[1].MaterialSlot == 1);

                    for (uint32_t index = 0; index < parts.getSize(); ++index)
                    {
                        const auto &mesh = parts[index].MeshResource;
                        success &= Check("imported part owns Mesh", mesh != nullptr);
                        if (!mesh)
                            continue;

                        const auto &indexBuffer = mesh->GetVertexArray().GetIndexBuffer();
                        success &= Check("imported triangle owns three indices",
                                         indexBuffer != nullptr && indexBuffer->GetCount() == 3);
                    }
                }
            }
        }

        // 同时检查创建与析构期间的 OpenGL 错误。
        const GLenum error = glGetError();
        success &= Check("OBJ import OpenGL state", error == GL_NO_ERROR);
        if (error != GL_NO_ERROR)
            std::cerr << "OpenGL error: " << error << '\n';
    }

    glfwMakeContextCurrent(nullptr);
    glfwDestroyWindow(window);
    glfwTerminate();

    if (success)
        std::cout << "Two-material OBJ import: PASS\n";
    return success ? 0 : 1;
}
