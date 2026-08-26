//
// Created by chenlong on 2026/8/26.
//

#pragma once

#include "Limen/Core/Core.h"

namespace Limen
{
    /**
     * @brief 后端无关的索引缓冲区接口。
     *
     * IndexBuffer 保存顶点索引以及索引数量。RendererAPI 会按照索引
     * 指定的顺序从 VertexBuffer 中取出顶点并组装图元。
     */
    class LIMEN_API IndexBuffer
    {
    public:
        virtual ~IndexBuffer() = default;

        IndexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        /**
         * @brief 根据当前 RendererAPI 创建对应后端的 IndexBuffer。
         *
         * @param indices CPU 端索引数组的首地址。
         * @param count 索引数量，不是数组占用的字节数。
         */
        [[nodiscard]]
        static IndexBuffer *Create(
            const uint32_t *indices,
            uint32_t count
        );

        /** @return 当前 IndexBuffer 保存的索引数量。 */
        [[nodiscard]]
        virtual uint32_t GetCount() const = 0;
    };
}
