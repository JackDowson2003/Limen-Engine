//
// Created by chenlong on 2026/8/11.
//
#include <glad/gl.h>
#include "OpenGLVertexArray.h"

#include "Log.h"
#include "OpenGLBuffer.h"

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

    void OpenGLVertexArray::AddVertexBuffer(const std::shared_ptr<VertexBuffer> &vertexBuffer)
    {
        OpenGLVertexArray::Bind();
        vertexBuffer->Bind(); //必须先bind 因为需要让后面的glVertexAttribPointer 接收到buffer ID

        LM_CORE_ASSERT(vertexBuffer->GetLayout().GetElements().size(), "Vertex Buffer has no layout!");

        const auto &layout = vertexBuffer->GetLayout();
        const int stride = static_cast<int>(layout.GetStride());
        uint32_t index = 0;
        for (const auto &element: layout)
        {
            glEnableVertexAttribArray(index);
            const GLenum type = ShaderDataTypeToOpenGLType(element.Type);
            const auto &count = static_cast<int>(element.GetComponentCount());
            glVertexAttribPointer(
                index,
                count,
                type,
                element.Normalized ? GL_TRUE : GL_FALSE,
                stride,
                reinterpret_cast<void *>(element.Offset)
            );
            index++;
        }
        m_VertexBuffers.push_back(vertexBuffer);
    }

    void OpenGLVertexArray::SetIndexBuffer(const std::shared_ptr<IndexBuffer> &indexBuffer)
    {
        OpenGLVertexArray::Bind();
        indexBuffer->Bind();

        m_IndexBuffer = indexBuffer;
    }

    const std::vector<std::shared_ptr<VertexBuffer>> & OpenGLVertexArray::GetVertexBuffers() const
    {
        return m_VertexBuffers;
    }

    const std::shared_ptr<IndexBuffer> & OpenGLVertexArray::GetIndexBuffer() const
    {
        return m_IndexBuffer;
    }
}
