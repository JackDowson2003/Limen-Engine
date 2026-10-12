//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include "Limen/RHI/VertexBuffer.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/Tool/LArray.h"

namespace Limen
{
    class LIMEN_API VertexArray
    {
    public:
        virtual ~VertexArray() = default;

        virtual void Bind() const = 0;

        virtual void UnBind() const = 0;

        virtual void AddVertexBuffer(const Ref<VertexBuffer> &vertexBuffer) = 0;

        virtual void SetIndexBuffer(const Ref<IndexBuffer> &indexBuffer) = 0;

        virtual const LArray<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const = 0;

        static VertexArray *Create();
    };
}
