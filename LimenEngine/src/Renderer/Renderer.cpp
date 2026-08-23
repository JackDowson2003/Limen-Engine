//
// Created by chenlong on 2026/8/11.
//
#include "Limen/Renderer/Renderer.h"

#include "Limen/Core/Log.h"

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

    void Renderer::OnWindowResize(const uint32_t width, const uint32_t height)
    {
        if (width == 0 || height == 0)
            return;

        RendererCommand::SetViewport(0, 0, width, height);
    }

    void Renderer::EndScene()
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ASSERT(false, "Renderer::EndScene called without a matching BeginScene");
            return;
        }

        // Present 属于窗口帧生命周期；EndScene() 只关闭逻辑提交区间。
        s_SceneData.IsActive = false;
    }

    void Renderer::Init()
    {
        RendererCommand::Init();
    }

    void Renderer::Shutdown()
    {
        RendererCommand::Shutdown();
    }

    // 兼容尚未迁移到 GraphicsPipeline 的 2D 示例。
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

        if (!shader)
        {
            LM_CORE_ASSERT(false, "Renderer::Submit received a null shader");
            return;
        }

        shader->Bind();

        shader->SetMat4(
            "u_ViewProjection",
            s_SceneData.ViewProjection
        );

        shader->SetMat4(
            "u_Transform",
            transform
        );

        shader->SetFloat3(
            "u_CameraPosition",
            s_SceneData.CameraPosition
        );

        vertexArray.Bind();
        RendererCommand::DrawIndexed(vertexArray);
    }

    void Renderer::Submit(
        const GraphicsPipeline &pipeline,
        const VertexArray &vertexArray,
        const glm::mat4 &transform
    )
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ERROR("Renderer::Submit must be called between BeginScene and EndScene");
            return;
        }

        const GraphicsPipelineSpecification &specification = pipeline.GetSpecification();

        if (!specification.ShaderProgram)
        {
            LM_CORE_ERROR("Renderer::Submit received a pipeline without a shader");
            return;
        }

        pipeline.Bind();

        const Ref<Shader> &shader = specification.ShaderProgram;

        shader->SetMat4(
            "u_ViewProjection",
            s_SceneData.ViewProjection
        );

        shader->SetMat4(
            "u_Transform",
            transform
        );

        shader->SetFloat3(
            "u_CameraPosition",
            s_SceneData.CameraPosition
        );

        vertexArray.Bind();
        RendererCommand::DrawIndexed(
            vertexArray,
            specification.Topology
        );
    }
}
