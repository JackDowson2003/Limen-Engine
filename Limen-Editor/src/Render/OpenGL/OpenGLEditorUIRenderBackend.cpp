//
// Created by chenlong on 2026/10/15.
//
#include "Render/OpenGL/OpenGLEditorUIRenderBackend.h"

#include <cstdint>

#include "Limen/Application/Window.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/VertexArray.h"
#include "Limen/Renderer/RendererCommand.h"

namespace LimenEditor
{
    namespace
    {
        constexpr char kVertexSource[] = R"(#version 410 core
            uniform vec2 u_WindowSize;
            uniform vec2 u_RectPosition;
            uniform vec2 u_RectSize;

            const vec2 kCorners[4] = vec2[4](
                vec2(0.0, 1.0),
                vec2(1.0, 1.0),
                vec2(1.0, 0.0),
                vec2(0.0, 0.0)
            );

            void main()
            {
                vec2 windowPosition =
                    u_RectPosition + kCorners[gl_VertexID] * u_RectSize;

                vec2 clipPosition = vec2(
                    2.0 * windowPosition.x / u_WindowSize.x - 1.0,
                    1.0 - 2.0 * windowPosition.y / u_WindowSize.y
                );

                gl_Position = vec4(clipPosition, 0.0, 1.0);
            })";

        constexpr char kFragmentSource[] = R"(#version 410 core
            uniform vec4 u_Color;
            layout(location = 0) out vec4 o_Color;

            void main()
            {
                o_Color = u_Color;
            })";
    }

    OpenGLEditorUIRenderBackend::~OpenGLEditorUIRenderBackend() = default;

    bool OpenGLEditorUIRenderBackend::Initialize()
    {
        m_VertexArray.reset(Limen::VertexArray::Create());
        if (!m_VertexArray)
        {
            LM_ERROR("Editor UI: failed to create vertex array");
            return false;
        }

        // OpenGL Core 使用 VAO 记录索引缓冲的关联。
        m_VertexArray->Bind();
        constexpr uint32_t indices[6] = {0, 1, 2, 2, 3, 0};
        Limen::Ref<Limen::IndexBuffer> ibo(
            Limen::IndexBuffer::Create(indices, 6)
        );

        if (!ibo)
        {
            LM_ERROR("Editor UI: failed to create index buffer");
            Shutdown();
            return false;
        }

        m_VertexArray->SetIndexBuffer(ibo);
        m_VertexArray->UnBind();

        const Limen::Ref<Limen::Shader> shader =
            Limen::Shader::CreateFromSource(
                "EditorUI/SolidRect",
                kVertexSource,
                kFragmentSource
            );

        if (!shader)
        {
            LM_ERROR("Editor UI: failed to create shader");
            Shutdown();
            return false;
        }

        Limen::GraphicsPipelineSpecification pipelineSpec;
        pipelineSpec.ShaderProgram = shader;
        pipelineSpec.DepthTestEnabled = false;
        pipelineSpec.DepthWriteEnabled = false;
        pipelineSpec.Blend = Limen::BlendMode::AlphaBlend;
        pipelineSpec.Culling = Limen::CullMode::None;
        pipelineSpec.DebugName = "Editor UI Solid Rect";

        m_Pipeline = Limen::GraphicsPipeline::Create(pipelineSpec);
        if (!m_Pipeline)
        {
            LM_ERROR("Editor UI: failed to create graphics pipeline");
            Shutdown();
            return false;
        }

        return true;
    }

    void OpenGLEditorUIRenderBackend::Shutdown()
    {
        m_Pipeline.reset();
        m_VertexArray.reset();
    }

    void OpenGLEditorUIRenderBackend::DrawSolidRect(
        const Limen::Window& window,
        const glm::vec2& position,
        const glm::vec2& size,
        const glm::vec4& color
    )
    {
        if (!m_Pipeline || !m_VertexArray ||
            size.x <= 0.0f || size.y <= 0.0f)
            return;

        const uint32_t fboWidth = window.GetWidth();
        const uint32_t fboHeight = window.GetHeight();
        const uint32_t windowWidth = window.GetWindowWidth();
        const uint32_t windowHeight = window.GetWindowHeight();

        if (fboWidth == 0 || fboHeight == 0 ||
            windowWidth == 0 || windowHeight == 0)
            return;

        // 逻辑窗口坐标用于 Shader；物理像素尺寸用于 GPU viewport。
        Limen::RendererCommand::SetViewport(0, 0, fboWidth, fboHeight);

        m_Pipeline->Bind();
        const auto& shader = m_Pipeline->GetSpecification().ShaderProgram;
        shader->SetFloat2(
            "u_WindowSize",
            glm::vec2(
                static_cast<float>(windowWidth),
                static_cast<float>(windowHeight)
            )
        );
        shader->SetFloat2("u_RectPosition", position);
        shader->SetFloat2("u_RectSize", size);
        shader->SetFloat4("u_Color", color);

        m_VertexArray->Bind();
        Limen::RendererCommand::DrawIndexed(
            *m_VertexArray,
            Limen::PrimitiveTopology::TriangleList,
            6
        );
    }
}
