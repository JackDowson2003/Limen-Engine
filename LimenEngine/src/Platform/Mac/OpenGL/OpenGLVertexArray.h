//
// Created by chenlong on 2026/8/11.
//
#pragma once
#include "Renderer/VertexArray.h"

namespace Limen
{
    class OpenGLVertexArray : public VertexArray
    {
    public:
        OpenGLVertexArray();

        ~OpenGLVertexArray() override;

        void Bind() const override;

        void UnBind() const override;

        void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) override;

        void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) override;

        const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const override;

        const Ref<IndexBuffer>& GetIndexBuffer() const override;

    private:
        uint32_t m_RendererID;
        std::vector<Ref<VertexBuffer>> m_VertexBuffers;
        Ref<IndexBuffer> m_IndexBuffer;
    };
}
