#pragma once


#include "Limen/Core/Core.h"

namespace Limen
{
    /**
     * @brief 跨图形API的常量缓冲区接口。
     *
     * OpenGL中对应Uniform Buffer Object；
     * D3D11/D3D12中对应Constant Buffer。
     */
    class LIMEN_API UniformBuffer
    {
    public:
        virtual ~UniformBuffer() = default;

        /**
         * @brief 更新UniformBuffer中的一段数据。
         *
         * @param data
         * CPU数据首地址，不能为nullptr。
         *
         * @param size
         * 本次写入的数据大小，单位为字节。
         *
         * @param offset
         * 从UniformBuffer的第几个字节开始写入，默认从0开始。
         */
        virtual void SetData(
            const void* data,
            uint32_t size,
            uint32_t offset = 0
        ) = 0;

        /**
         * @return 创建时分配的总字节数。
         */
        [[nodiscard]]
        virtual uint32_t GetSize() const noexcept = 0;

        /**
         * @return Shader与Buffer之间约定的绑定位置。
         */
        [[nodiscard]]
        virtual uint32_t GetBinding() const noexcept = 0;

        /**
         * @brief 根据当前RendererAPI创建对应后端的UniformBuffer。
         *
         * @param size
         * Buffer总大小，单位为字节，必须大于0。
         *
         * @param binding
         * Shader绑定位置。例如：
         * 0表示Scene，1表示Object，2表示Material。
         */
        [[nodiscard]]
        static Scope<UniformBuffer> Create(
            uint32_t size,
            uint32_t binding
        );
    };
}