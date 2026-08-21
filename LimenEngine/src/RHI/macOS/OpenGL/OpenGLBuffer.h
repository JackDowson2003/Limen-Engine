//
// Created by chenlong on 2026/8/11.
//

#pragma once

#include <glad/gl.h>

#include "Limen/RHI/Buffer.h"

namespace Limen
{
    // 将跨平台的 ShaderDataType 映射为 OpenGL 顶点属性的基础存储类型。
    // None 和 Bool 不存在合法映射，函数会报告错误并返回 GL_NONE。
    [[nodiscard]] GLenum  ShaderDataTypeToOpenGLType(ShaderDataType type);


    class LIMEN_API OpenGLVertexBuffer : public VertexBuffer
    {
    public:
        OpenGLVertexBuffer(const void *vertices, uint32_t size);

        ~OpenGLVertexBuffer() override;
        OpenGLVertexBuffer(OpenGLVertexBuffer&&) noexcept;
        OpenGLVertexBuffer& operator=(OpenGLVertexBuffer&&) noexcept;
        void SetLayout(const BufferLayout &layout) override
        {
            m_Layout = layout;
        }

        const BufferLayout &GetLayout() const override
        {
            return m_Layout;
        }


        void Bind() const override;

        void UnBind() const override;

    private:
        uint32_t m_RendererID{}; //buffer ID
        BufferLayout m_Layout;
    };

    class LIMEN_API OpenGLIndexBuffer : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(const uint32_t *indices, uint32_t count);

        OpenGLIndexBuffer(OpenGLIndexBuffer&&) noexcept;
        OpenGLIndexBuffer& operator=(OpenGLIndexBuffer&&) noexcept;

        ~OpenGLIndexBuffer() override; //不能是default
        void Bind() const override;

        void UnBind() const override;

        [[nodiscard]] uint32_t GetCount() const override;

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_Count;
    };
}
