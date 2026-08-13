//
// Created by chenlong on 2026/8/11.
//

#include <glad/gl.h>

#include "OpenGLBuffer.h"
#include "Log.h"

namespace Limen
{
    GLenum ShaderDataTypeToOpenGLType(const ShaderDataType type)
    {
        switch (type)
        {
            case ShaderDataType::Float:
            case ShaderDataType::Float2:
            case ShaderDataType::Float3:
            case ShaderDataType::Float4:
            case ShaderDataType::FMat2:
            case ShaderDataType::FMat3:
            case ShaderDataType::FMat4:
                return GL_FLOAT;

            case ShaderDataType::Half:
            case ShaderDataType::Half2:
            case ShaderDataType::Half3:
            case ShaderDataType::Half4:
                return GL_HALF_FLOAT;

            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
                return GL_INT;

            case ShaderDataType::UInt:
            case ShaderDataType::UInt2:
            case ShaderDataType::UInt3:
            case ShaderDataType::UInt4:
                return GL_UNSIGNED_INT;

            case ShaderDataType::Byte:
            case ShaderDataType::Byte2:
            case ShaderDataType::Byte3:
            case ShaderDataType::Byte4:
                return GL_BYTE;

            case ShaderDataType::UByte:
            case ShaderDataType::UByte2:
            case ShaderDataType::UByte3:
            case ShaderDataType::UByte4:
                return GL_UNSIGNED_BYTE;

            case ShaderDataType::Short:
            case ShaderDataType::Short2:
            case ShaderDataType::Short3:
            case ShaderDataType::Short4:
                return GL_SHORT;

            case ShaderDataType::UShort:
            case ShaderDataType::UShort2:
            case ShaderDataType::UShort3:
            case ShaderDataType::UShort4:
                return GL_UNSIGNED_SHORT;

            case ShaderDataType::Double:
            case ShaderDataType::Double2:
            case ShaderDataType::Double3:
            case ShaderDataType::Double4:
            case ShaderDataType::DMat2:
            case ShaderDataType::DMat3:
            case ShaderDataType::DMat4:
                return GL_DOUBLE;

            case ShaderDataType::None:
                LM_CORE_ERROR("ShaderDataType::None cannot be used as an OpenGL vertex attribute");
                return GL_NONE;

            case ShaderDataType::Bool:
                LM_CORE_ERROR("OpenGL vertex attributes do not support ShaderDataType::Bool; use UByte instead");
                return GL_NONE;
        }

        LM_CORE_ERROR("Unknown ShaderDataType cannot be mapped to OpenGL");
        return GL_NONE;
    }

    ///VBO============================================================
    OpenGLVertexBuffer::OpenGLVertexBuffer(const void *vertices, const uint32_t size)
    {
        glGenBuffers(1, &m_RendererID);
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
        glBufferData(GL_ARRAY_BUFFER, size, vertices,GL_STATIC_DRAW);
    }

    OpenGLVertexBuffer::~OpenGLVertexBuffer()
    {
        glDeleteBuffers(1, &m_RendererID);
    }

    OpenGLVertexBuffer::OpenGLVertexBuffer(OpenGLVertexBuffer &&vb) noexcept
        :m_RendererID(vb.m_RendererID),m_Layout(std::move(vb.m_Layout))
    {
        vb.m_RendererID = 0;
    }

    OpenGLVertexBuffer & OpenGLVertexBuffer::operator=(OpenGLVertexBuffer &&vb) noexcept
    {
        if (this == &vb)
        {
            return *this;
        }
        m_RendererID = vb.m_RendererID;
        m_Layout = std::move(vb.m_Layout);
        return *this;
    }


    void OpenGLVertexBuffer::Bind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, m_RendererID);
    }

    void OpenGLVertexBuffer::UnBind() const
    {
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    ///VBO END

    ///IBO============================================================
    OpenGLIndexBuffer::OpenGLIndexBuffer(const uint32_t *indices, const uint32_t count)
        : m_Count(count)
    {
        glGenBuffers(1, &m_RendererID);
        OpenGLIndexBuffer::Bind();
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(uint32_t) * count, indices,GL_STATIC_DRAW);
    }

    OpenGLIndexBuffer::OpenGLIndexBuffer(OpenGLIndexBuffer &&ib) noexcept
        :m_RendererID(ib.m_RendererID),m_Count(ib.m_Count)
    {
        ib.m_RendererID = 0;
        ib.m_Count = 0;
    }

    OpenGLIndexBuffer & OpenGLIndexBuffer::operator=(OpenGLIndexBuffer &&ib) noexcept
    {
        if (this == &ib)
        {
            return *this;
        }
        m_RendererID = ib.m_RendererID;
        m_Count = ib.m_Count;
        ib.m_RendererID = 0;
        ib.m_Count = 0;
        return *this;
    }

    OpenGLIndexBuffer::~OpenGLIndexBuffer()
    {
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

    ///IBO============================================================ END
}
