//
// Created by chenlong on 2026/10/15.
//
#include "Render/OpenGL/OpenGLEditorUIRenderBackend.h"

#include <cstdint>

#include "Limen/Application/Window.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/VertexArray.h"
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/Texture.h"

namespace LimenEditor
{
    namespace
    {
        constexpr char kVertexSource[] = R"(#version 410 core
            uniform vec2 u_WindowSize;
            uniform vec2 u_RectPosition;
            uniform vec2 u_RectSize;

            out vec2 v_TexCoord;

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
                v_TexCoord = vec2(kCorners[gl_VertexID].x, 1.0 - kCorners[gl_VertexID].y);
            })";

        constexpr char kFragmentSource[] = R"(#version 410 core
            uniform vec4 u_Color;
            layout(location = 0) out vec4 o_Color;

            void main()
            {
                o_Color = u_Color;
            })";

        constexpr char kTexturedFragmentSource[] = R"(#version 410 core
            in vec2 v_TexCoord;
            uniform sampler2D u_Texture;
            layout(location = 0) out vec4 o_Color;

            float LinearChannelToSRGB(float value)
            {
                value = clamp(value, 0.0, 1.0);
                if (value <= 0.0031308)
                    return value * 12.92;
                return 1.055 * pow(value, 1.0 / 2.4) - 0.055;
            }

            void main()
            {
                vec4 sampled = texture(u_Texture, v_TexCoord);
                // sRGB 纹理采样得到线性 RGB；当前窗口显示路径在 Shader 中编码。
                // Alpha 不是颜色亮度，保持原值。
                o_Color = vec4(
                    LinearChannelToSRGB(sampled.r),
                    LinearChannelToSRGB(sampled.g),
                    LinearChannelToSRGB(sampled.b),
                    sampled.a
                );
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
        const Limen::Ref<Limen::IndexBuffer> ibo(
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

        const Limen::Ref<Limen::Shader> &texturedShader =
                Limen::Shader::CreateFromSource("EditorUI/TexturedRect", kVertexSource, kTexturedFragmentSource);

        if (!texturedShader)
        {
            LM_ERROR("Editor UI: failed to create textured shader");
            Shutdown();
            return false;
        }
        Limen::GraphicsPipelineSpecification texturedSpec = pipelineSpec;
        texturedSpec.ShaderProgram = texturedShader;
        texturedSpec.DebugName = "Editor UI Textured Rect";
        m_TexturedPipeline = Limen::GraphicsPipeline::Create(texturedSpec);

        if (!m_TexturedPipeline)
        {
            LM_ERROR("Editor UI: failed to create textured pipeline");
            Shutdown();
            return false;
        }
        return true;
    }

    void OpenGLEditorUIRenderBackend::Shutdown()
    {
        m_TexturedPipeline.reset();
        m_Pipeline.reset();
        m_VertexArray.reset();
    }

    void OpenGLEditorUIRenderBackend::DrawSolidRect(
        const Limen::Window &window,
        const glm::vec2 &position,
        const glm::vec2 &size,
        const glm::vec4 &color
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
        const auto &shader = m_Pipeline->GetSpecification().ShaderProgram;
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

    void OpenGLEditorUIRenderBackend::DrawTexturedRect(
        const Limen::Window &window, const glm::vec2 &position,
        const glm::vec2 &size, const Limen::Texture2D &texture)
    {
        if (!m_TexturedPipeline || !m_VertexArray || size.x <= 0.0f || size.y <= 0)
            return;

        const uint32_t fboWidth = window.GetWidth();
        const uint32_t fboHeight = window.GetHeight();
        const uint32_t windowWidth = window.GetWindowWidth();
        const uint32_t windowHeight = window.GetWindowHeight();

        if (fboWidth == 0 || fboHeight == 0 || windowWidth == 0 || windowHeight == 0)
            return;

        Limen::RendererCommand::SetViewport(0, 0, fboWidth, fboHeight);
        m_TexturedPipeline->Bind();
        const auto &shader = m_TexturedPipeline->GetSpecification().ShaderProgram;
        shader->SetFloat2("u_WindowSize", {
                              static_cast<float>(windowWidth),
                              static_cast<float>(windowHeight)
                          });
        shader->SetFloat2("u_RectPosition", position);
        shader->SetFloat2("u_RectSize", size);
        shader->SetInt("u_Texture", 0);
        texture.Bind(0);
        m_VertexArray->Bind();
        Limen::RendererCommand::DrawIndexed(
            *m_VertexArray,
            Limen::PrimitiveTopology::TriangleList,
            6
        );
    }
}
