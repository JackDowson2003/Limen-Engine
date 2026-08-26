//
// Created by chenlong on 2026/8/11.
//
#include "Limen/Renderer/Renderer2D.h"
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
        /*
         * 先创建并初始化底层 RendererAPI。
         *
         * Renderer2D 创建 VAO、VBO、Shader、UBO 和 Pipeline 时，
         * 会通过 RendererCommand 和 RHI 使用当前图形 API。
         */
        RendererCommand::Init();

        /*
         * 底层渲染后端和 GraphicsContext 已经可用，
         * 现在可以创建 Renderer2D 的公共 GPU 资源。
         */
        Renderer2D::Init();
    }

    void Renderer::Shutdown()
    {
        /*
         * 先释放依赖底层图形 API 的二维 GPU 资源。
         */
        Renderer2D::Shutdown();

        /*
         * 所有高层渲染资源释放后，再销毁底层 RendererAPI。
         */
        RendererCommand::Shutdown();
    }

    // 兼容尚未迁移到 GraphicsPipeline 的 2D 示例。
    void Renderer::Submit(
        const Ref<Shader> &shader,
        const VertexArray &vertexArray,
        const glm::mat4 &transform
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
