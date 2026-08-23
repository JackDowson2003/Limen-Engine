//
// Created by chenlong on 2026/8/23.
//
#include "OpenGLGraphicsPipeline.h"

#include <glad/gl.h>

#include "Limen/Core/Log.h"

namespace Limen
{
    namespace
    {
        GLenum ToOpenGLCompareOperation(const CompareOperation operation)
        {
            switch (operation)
            {
                case CompareOperation::Never:
                    return GL_NEVER;

                case CompareOperation::Less:
                    return GL_LESS;

                case CompareOperation::LessEqual:
                    return GL_LEQUAL;

                case CompareOperation::Equal:
                    return GL_EQUAL;

                case CompareOperation::Greater:
                    return GL_GREATER;

                case CompareOperation::GreaterEqual:
                    return GL_GEQUAL;

                case CompareOperation::NotEqual:
                    return GL_NOTEQUAL;

                case CompareOperation::Always:
                    return GL_ALWAYS;
            }

            LM_CORE_ERROR("Unknown depth comparison operation");

            return GL_LESS;
        }

        /** @brief 将公共正面绕序转换为 OpenGL 枚举。 */
        GLenum ToOpenGLFrontFace(const FrontFace frontFace)
        {
            switch (frontFace)
            {
                case FrontFace::CounterClockwise:
                    return GL_CCW;

                case FrontFace::Clockwise:
                    return GL_CW;
            }

            LM_CORE_ERROR("Unknown front-face winding");

            return GL_CCW;
        }

        /** @brief 应用颜色混合模式及其对应的混合因子。 */
        void ApplyOpenGLBlendMode(const BlendMode blendMode)
        {
            switch (blendMode)
            {
                case BlendMode::Opaque:
                {
                    glDisable(GL_BLEND);
                    return;
                }

                case BlendMode::AlphaBlend:
                {
                    glEnable(GL_BLEND);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
                    return;
                }

                case BlendMode::Additive:
                {
                    glEnable(GL_BLEND);
                    glBlendEquation(GL_FUNC_ADD);
                    glBlendFunc(GL_SRC_ALPHA, GL_ONE);

                    return;
                }
            }

            LM_CORE_ERROR("Unknown blend mode");
        }

        /** @brief 应用三角形面剔除状态。 */
        void ApplyOpenGLCullMode(const CullMode cullMode)
        {
            switch (cullMode)
            {
                case CullMode::None:
                {
                    glDisable(GL_CULL_FACE);
                    return;
                }

                case CullMode::Front:
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_FRONT);
                    return;
                }

                case CullMode::Back:
                {
                    glEnable(GL_CULL_FACE);
                    glCullFace(GL_BACK);
                    return;
                }
            }

            LM_CORE_ERROR("Unknown cull mode");
        }
    }

    OpenGLGraphicsPipeline::OpenGLGraphicsPipeline(const GraphicsPipelineSpecification &specification)
        : m_Specification(specification)
    {
        LM_CORE_ASSERT(
            m_Specification.ShaderProgram,
            "OpenGL graphics pipeline '{}' requires a valid shader",
            m_Specification.DebugName
        );

        if (!m_Specification.ShaderProgram)
        {
            LM_CORE_ERROR(
                "Failed to create OpenGL graphics pipeline '{}': shader is null",
                m_Specification.DebugName
            );
        }
    }

    void OpenGLGraphicsPipeline::Bind() const
    {
        if (!m_Specification.ShaderProgram)
        {
            LM_CORE_ERROR("Cannot bind OpenGL graphics pipeline without a shader");
            return;
        }
        // Shader 必须先绑定，后续 Set* Uniform 调用才会写入正确的 Program。
        m_Specification.ShaderProgram->Bind();

        if (m_Specification.DepthTestEnabled)
        {
            glEnable(GL_DEPTH_TEST);
            glDepthFunc(ToOpenGLCompareOperation(m_Specification.DepthCompare));
        }
        else
        {
            glDisable(GL_DEPTH_TEST);
        }

        // 深度测试与深度写入是独立状态，透明物体通常测试但不写入。
        glDepthMask(m_Specification.DepthWriteEnabled ? GL_TRUE : GL_FALSE);

        ApplyOpenGLBlendMode(m_Specification.Blend);

        ApplyOpenGLCullMode(m_Specification.Culling);

        glFrontFace(ToOpenGLFrontFace(m_Specification.FrontFaceWinding));

        // PrimitiveTopology 在 glDrawElements() 时应用，不属于此处的状态绑定。
    }
}
