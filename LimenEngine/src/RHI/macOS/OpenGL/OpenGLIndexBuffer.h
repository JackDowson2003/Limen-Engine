//
// Created by chenlong on 2026/8/26.
//

#pragma once

#include "Limen/RHI/IndexBuffer.h"

namespace Limen
{
    /** @brief macOS OpenGL 后端的索引缓冲区实现。 */
    class OpenGLIndexBuffer final : public IndexBuffer
    {
    public:
        OpenGLIndexBuffer(
            const uint32_t *indices,
            uint32_t count
        );

        OpenGLIndexBuffer(OpenGLIndexBuffer &&other) noexcept;
        OpenGLIndexBuffer &operator=(OpenGLIndexBuffer &&other) noexcept;

        ~OpenGLIndexBuffer() override;

        void Bind() const override;
        void UnBind() const override;

        [[nodiscard]]
        uint32_t GetCount() const override;

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_Count = 0;
    };
}
