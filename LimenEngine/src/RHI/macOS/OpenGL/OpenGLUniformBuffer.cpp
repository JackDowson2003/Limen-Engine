#include "OpenGLUniformBuffer.h"

#include <glad/gl.h>

#include "Limen/Core/Log.h"

namespace Limen
{
    OpenGLUniformBuffer::OpenGLUniformBuffer(
        const uint32_t size,
        const uint32_t binding
    )
        : m_Size(size),
          m_Binding(binding)
    {
        LM_CORE_ASSERT(
            size > 0,
            "UniformBuffer size must be greater than zero"
        );

        if (size == 0)
            return;

        // 创建OpenGL Buffer对象。
        glGenBuffers(1, &m_RendererID);

        // 当前操作目标是Uniform Buffer。
        glBindBuffer(
            GL_UNIFORM_BUFFER,
            m_RendererID
        );

        // 只分配GPU内存，初始阶段不上传数据。
        glBufferData(
            GL_UNIFORM_BUFFER,
            static_cast<GLsizeiptr>(m_Size),
            nullptr,
            GL_DYNAMIC_DRAW
        );

        // 将Buffer连接到指定的Uniform Buffer绑定位置。
        glBindBufferBase(
            GL_UNIFORM_BUFFER,
            m_Binding,
            m_RendererID
        );

        // 避免后续代码意外修改当前Buffer。
        glBindBuffer(
            GL_UNIFORM_BUFFER,
            0
        );
    }

    OpenGLUniformBuffer::~OpenGLUniformBuffer()
    {
        if (m_RendererID != 0)
            glDeleteBuffers(1, &m_RendererID);
    }

    void OpenGLUniformBuffer::SetData(
        const void* data,
        const uint32_t size,
        const uint32_t offset
    )
    {
        LM_CORE_ASSERT(
            data,
            "UniformBuffer data cannot be null"
        );

        // 使用减法检查，避免offset + size发生整数溢出。
        const bool isRangeValid =
            offset <= m_Size &&
            size <= m_Size - offset;

        LM_CORE_ASSERT(
            isRangeValid,
            "UniformBuffer write range is out of bounds"
        );

        if (!data || !isRangeValid)
            return;

        glBindBuffer(
            GL_UNIFORM_BUFFER,
            m_RendererID
        );

        glBufferSubData(
            GL_UNIFORM_BUFFER,
            static_cast<GLintptr>(offset),
            static_cast<GLsizeiptr>(size),
            data
        );

        glBindBuffer(
            GL_UNIFORM_BUFFER,
            0
        );
    }
}