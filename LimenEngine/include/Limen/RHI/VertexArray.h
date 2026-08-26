//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include <vector>

#include "Limen/RHI/Buffer.h"

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

        virtual const std::vector<Ref<VertexBuffer>>& GetVertexBuffers() const = 0;
        virtual const Ref<IndexBuffer>& GetIndexBuffer() const = 0;

        /**
         * @brief 更新 VertexBuffer 中的一段顶点数据。
         *
         * @param data CPU 顶点数据首地址。
         * @param size 本次上传的数据大小，单位为字节。
         * @param offset 从 Buffer 的第几个字节开始写入。
         */
        virtual void SetData(
            const void *data,
            uint32_t size,
            uint32_t offset = 0
        ) = 0;

        /**
         * @brief 创建一个只分配容量、不提供初始数据的动态 VertexBuffer。
         *
         * @param size Buffer 的总容量，单位为字节。
         */
        static VertexBuffer *Create(uint32_t size);

        static VertexArray *Create();
    };
}
