//
// Created by chenlong on 2026/8/11.
//
#include <glad/gl.h>
#include "OpenGLVertexArray.h"

#include "Limen/Core/Log.h"
#include "OpenGLVertexBuffer.h"

namespace Limen
{
    OpenGLVertexArray::OpenGLVertexArray()
    {
        glGenVertexArrays(1, &m_RendererID);
    }

    OpenGLVertexArray::~OpenGLVertexArray()
    {
        OpenGLVertexArray::UnBind();
        glDeleteVertexArrays(1, &m_RendererID);
    }

    void OpenGLVertexArray::Bind() const
    {
        glBindVertexArray(m_RendererID);
    }

    void OpenGLVertexArray::UnBind() const
    {
        glBindVertexArray(0);
    }

    void OpenGLVertexArray::AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer)
    {
        OpenGLVertexArray::Bind();
        // glVertexAttribPointer 会把当前 GL_ARRAY_BUFFER 记录进 VAO 状态。
        vertexBuffer->Bind();

        LM_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

        const auto &layout = vertexBuffer->GetLayout();
        const int stride = static_cast<int>(layout.GetStride());
        uint32_t index = 0;
        for (const auto &element: layout)
        {
            glEnableVertexAttribArray(index);
            const GLenum type = ShaderDataTypeToOpenGLType(element.Type);
            const GLint size = static_cast<int>(element.GetComponentSize());
            // glVertexAttribPointer(
            //     index,
            //     size,
            //     type,
            //     element.Normalized ? GL_TRUE : GL_FALSE,
            //     stride,
            //     reinterpret_cast<void *>(element.Offset)
            // );
            if (ShaderDataTypeIsInteger(element.Type) && !element.Normalized)
            {
                // Int、UInt 等整数属性必须使用整数读取接口，
                // 这样 GLSL 才能使用 int/uint 接收原始整数值。
                glVertexAttribIPointer(
                    index,
                    size,
                    type,
                    stride,
                    reinterpret_cast<const void *>(element.Offset)
                );
            } else
            {
                // Float 属性以及需要归一化为浮点数的整数存储走普通接口。
                glVertexAttribPointer(
                    index,
                    size,
                    type,
                    element.Normalized ? GL_TRUE : GL_FALSE,
                    stride,
                    reinterpret_cast<const void *>(element.Offset)
                );
            }
            index++;
        }
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer)
    {
        OpenGLVertexArray::Bind();
        indexBuffer->Bind();

        m_IndexBuffer = indexBuffer;
    }

    const std::vector<Ref<VertexBuffer> > &OpenGLVertexArray::GetVertexBuffers() const
    {
        return m_VertexBuffers;
    }

    const Ref<IndexBuffer> &OpenGLVertexArray::GetIndexBuffer() const
    {
        return m_IndexBuffer;
    }
}
