#pragma once

#include <cstdint>

#include "Limen/RHI/UniformBuffer.h"

namespace Limen
{
    /**
     * @brief macOS OpenGL 4.1的UniformBuffer实现。
     */
    class OpenGLUniformBuffer final : public UniformBuffer
    {
    public:
        OpenGLUniformBuffer(
            uint32_t size,
            uint32_t binding
        );

        ~OpenGLUniformBuffer() override;

        OpenGLUniformBuffer(
            const OpenGLUniformBuffer&
        ) = delete;

        OpenGLUniformBuffer& operator=(
            const OpenGLUniformBuffer&
        ) = delete;

        void SetData(
            const void* data,
            uint32_t size,
            uint32_t offset = 0
        ) override;

        [[nodiscard]]
        uint32_t GetSize() const noexcept override
        {
            return m_Size;
        }

        [[nodiscard]]
        uint32_t GetBinding() const noexcept override
        {
            return m_Binding;
        }

    private:
        uint32_t m_RendererID = 0;
        uint32_t m_Size = 0;
        uint32_t m_Binding = 0;
    };
}