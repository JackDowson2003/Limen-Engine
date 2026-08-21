//
// Created by chenlong on 2026/8/11.
//
#include "Renderer/Renderer.h"

#include "Log.h"
#include "Platform/Mac/OpenGL/OpenGLShader.h"

namespace Limen
{
    namespace
    {
        struct SceneData
        {
            bool IsActive = false;
            glm::mat4 ViewProjection{1.0f};
            glm::vec3 CameraPosition{0.0f};
        };

        SceneData s_SceneData;
    }

    // void Renderer::BeginScene(const OrthoGraphicCamera &camera)
    // {
    //     if (s_SceneData.IsActive)
    //     {
    //         LM_CORE_ASSERT(false, "Renderer::BeginScene cannot be called while another scene is active");
    //         return;
    //     }
    //     s_SceneData.ViewProjectionMatrix = camera.GetViewProjectionMatrix();
    //
    //     s_SceneData.IsActive = true;
    // }

    void Renderer::BeginScene(const Camera &camera)
    {

        if (s_SceneData.IsActive)
        {
            LM_CORE_ERROR("Another scene is already active");
            return;
        }
        s_SceneData.ViewProjection =
            camera.GetViewProjectionMatrix();

        s_SceneData.CameraPosition =
            camera.GetPosition();

        s_SceneData.IsActive = true;
    }

    void Renderer::EndScene()
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ASSERT(false, "Renderer::EndScene called without a matching BeginScene");
            return;
        }

        // EndScene 结束的是逻辑提交区间。交换缓冲仍由 Window::OnUpdate 负责，
        // 因为 Present/SwapBuffers 属于窗口与帧生命周期，而不是场景本身。
        s_SceneData.IsActive = false;
    }

    void Renderer::Init()
    {
        RendererCommand::Init();
    }

    //OpenGL Submit
    //Just Get Resources to use by displaying
    void Renderer::Submit(
        const Ref<Shader> &shader,
        const VertexArray &vertexArray,
        const glm::mat4& transform

        )
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ASSERT(false, "Renderer::Submit must be called between BeginScene and EndScene");
            return;
        }

        // Submit 只借用资源，不参与 Shader 和 VertexArray 的所有权。
        shader->Bind();
        std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_ViewProjection", s_SceneData.ViewProjection);
        std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformMat4("u_Transform", transform);
        // Blinn-Phong需要从当前着色点指向相机的观察方向。
        // BeginScene已从Camera中保存了相机世界坐标，此处上传给Shader。
        std::dynamic_pointer_cast<OpenGLShader>(shader)->UploadUniformFloat3(
            "u_CameraPosition",
            s_SceneData.CameraPosition
        );
        vertexArray.Bind();
        //底层命令
        RendererCommand::DrawIndexed(vertexArray);
    }
}
