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

        /**
         * @brief 创建指定容量的动态 OpenGL VertexBuffer。
         */
        explicit OpenGLVertexBuffer(uint32_t size);

        void SetData(
            const void *data,
            uint32_t size,
            uint32_t offset = 0
        ) override;

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
        uint32_t m_RendererID = 0;
        BufferLayout m_Layout;

        /**
         * @brief 当前VBO分配的总字节
         *
         * SetData() 使用他检查写入范围是否越界
         */
        uint32_t m_Size = 0;
    };

}
