//
// Created by chenlong on 2026/8/26.
//

#include "OpenGLIndexBuffer.h"

#include <glad/gl.h>

namespace Limen
{
    OpenGLIndexBuffer::OpenGLIndexBuffer(
        const uint32_t *indices,
        const uint32_t count
    )
        : m_Count(count)
    {
        glGenBuffers(1, &m_RendererID);
        Bind();
        glBufferData(
            GL_ELEMENT_ARRAY_BUFFER,
            static_cast<GLsizeiptr>(sizeof(uint32_t) * count),
            indices,
            GL_STATIC_DRAW
        );
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(
        OpenGLIndexBuffer &&other
    ) noexcept
        : m_RendererID(other.m_RendererID),
          m_Count(other.m_Count)
    {
        other.m_RendererID = 0;
        other.m_Count = 0;
    }

    OpenGLIndexBuffer &OpenGLIndexBuffer::operator=(
        OpenGLIndexBuffer &&other
    ) noexcept
    {
        if (this == &other)
            return *this;

        if (m_RendererID)
            glDeleteBuffers(1, &m_RendererID);

        m_RendererID = other.m_RendererID;
        m_Count = other.m_Count;

        other.m_RendererID = 0;
        other.m_Count = 0;

        return *this;
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
        if (m_RendererID)
            glDeleteBuffers(1, &m_RendererID);
    }

    void OpenGLIndexBuffer::Bind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, m_RendererID);
    }

    void OpenGLIndexBuffer::UnBind() const
    {
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
    }

    uint32_t OpenGLIndexBuffer::GetCount() const
    {
        return m_Count;
    }
}
