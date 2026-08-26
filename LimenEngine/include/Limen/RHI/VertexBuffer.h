//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include <initializer_list>
#include <string>
#include <vector>

#include "Limen/Core/Core.h"


namespace Limen
{
    // 描述顶点缓冲区中的数据格式，不包含任何 OpenGL/DX12 专属枚举值。
    // 具体后端仍可拒绝自己无法直接表示的格式。
    enum class ShaderDataType : uint32_t
    {
        None = 0,

        Float, Float2, Float3, Float4,
        FMat2, FMat3, FMat4,

        // Half 表示 IEEE 754 16 位浮点数的原始存储，不是 C++ 的 float。
        Half, Half2, Half3, Half4,

        Int, Int2, Int3, Int4,
        UInt, UInt2, UInt3, UInt4,

        Byte, Byte2, Byte3, Byte4,
        UByte, UByte2, UByte3, UByte4,

        Short, Short2, Short3, Short4,
        UShort, UShort2, UShort3, UShort4,

        Double, Double2, Double3, Double4,
        DMat2, DMat3, DMat4,

        // 保留用于数据描述；OpenGL 顶点属性不支持 GL_BOOL。
        Bool,
    };

    // 返回紧密排列时占用的总字节数，不包含 std140/std430 等额外对齐。
    [[nodiscard]] constexpr uint32_t LIMEN_API ShaderDataTypeSize(const ShaderDataType type) noexcept
    {
        switch (type)
        {
            case ShaderDataType::None: return 0;

            case ShaderDataType::Float:  return sizeof(float);
            case ShaderDataType::Float2: return sizeof(float) * 2;
            case ShaderDataType::Float3: return sizeof(float) * 3;
            case ShaderDataType::Float4: return sizeof(float) * 4;
            case ShaderDataType::FMat2:  return sizeof(float) * 2 * 2;
            case ShaderDataType::FMat3:  return sizeof(float) * 3 * 3;
            case ShaderDataType::FMat4:  return sizeof(float) * 4 * 4;

            case ShaderDataType::Half:  return sizeof(uint16_t);
            case ShaderDataType::Half2: return sizeof(uint16_t) * 2;
            case ShaderDataType::Half3: return sizeof(uint16_t) * 3;
            case ShaderDataType::Half4: return sizeof(uint16_t) * 4;

            case ShaderDataType::Int:  return sizeof(int32_t);
            case ShaderDataType::Int2: return sizeof(int32_t) * 2;
            case ShaderDataType::Int3: return sizeof(int32_t) * 3;
            case ShaderDataType::Int4: return sizeof(int32_t) * 4;

            case ShaderDataType::UInt:  return sizeof(uint32_t);
            case ShaderDataType::UInt2: return sizeof(uint32_t) * 2;
            case ShaderDataType::UInt3: return sizeof(uint32_t) * 3;
            case ShaderDataType::UInt4: return sizeof(uint32_t) * 4;

            case ShaderDataType::Byte:  return sizeof(int8_t);
            case ShaderDataType::Byte2: return sizeof(int8_t) * 2;
            case ShaderDataType::Byte3: return sizeof(int8_t) * 3;
            case ShaderDataType::Byte4: return sizeof(int8_t) * 4;

            case ShaderDataType::UByte:  return sizeof(uint8_t);
            case ShaderDataType::UByte2: return sizeof(uint8_t) * 2;
            case ShaderDataType::UByte3: return sizeof(uint8_t) * 3;
            case ShaderDataType::UByte4: return sizeof(uint8_t) * 4;

            case ShaderDataType::Short:  return sizeof(int16_t);
            case ShaderDataType::Short2: return sizeof(int16_t) * 2;
            case ShaderDataType::Short3: return sizeof(int16_t) * 3;
            case ShaderDataType::Short4: return sizeof(int16_t) * 4;

            case ShaderDataType::UShort:  return sizeof(uint16_t);
            case ShaderDataType::UShort2: return sizeof(uint16_t) * 2;
            case ShaderDataType::UShort3: return sizeof(uint16_t) * 3;
            case ShaderDataType::UShort4: return sizeof(uint16_t) * 4;

            case ShaderDataType::Double:  return sizeof(double);
            case ShaderDataType::Double2: return sizeof(double) * 2;
            case ShaderDataType::Double3: return sizeof(double) * 3;
            case ShaderDataType::Double4: return sizeof(double) * 4;
            case ShaderDataType::DMat2:   return sizeof(double) * 2 * 2;
            case ShaderDataType::DMat3:   return sizeof(double) * 3 * 3;
            case ShaderDataType::DMat4:   return sizeof(double) * 4 * 4;

            // Bool 仅表示一个稳定的一字节 CPU 存储格式。
            case ShaderDataType::Bool: return sizeof(uint8_t);
        }

        return 0;
    }

    // 返回一个 attribute location 中的分量数，合法值为 1~4。
    // 矩阵会拆成多列，每一列分别占用一个 location。
    [[nodiscard]] constexpr uint32_t ShaderDataTypeComponentCount(const ShaderDataType type) noexcept
    {
        switch (type)
        {
            case ShaderDataType::None: return 0;

            case ShaderDataType::Float:
            case ShaderDataType::Half:
            case ShaderDataType::Int:
            case ShaderDataType::UInt:
            case ShaderDataType::Byte:
            case ShaderDataType::UByte:
            case ShaderDataType::Short:
            case ShaderDataType::UShort:
            case ShaderDataType::Double:
            case ShaderDataType::Bool:
                return 1;

            case ShaderDataType::Float2:
            case ShaderDataType::FMat2:
            case ShaderDataType::Half2:
            case ShaderDataType::Int2:
            case ShaderDataType::UInt2:
            case ShaderDataType::Byte2:
            case ShaderDataType::UByte2:
            case ShaderDataType::Short2:
            case ShaderDataType::UShort2:
            case ShaderDataType::Double2:
            case ShaderDataType::DMat2:
                return 2;

            case ShaderDataType::Float3:
            case ShaderDataType::FMat3:
            case ShaderDataType::Half3:
            case ShaderDataType::Int3:
            case ShaderDataType::UInt3:
            case ShaderDataType::Byte3:
            case ShaderDataType::UByte3:
            case ShaderDataType::Short3:
            case ShaderDataType::UShort3:
            case ShaderDataType::Double3:
            case ShaderDataType::DMat3:
                return 3;

            case ShaderDataType::Float4:
            case ShaderDataType::FMat4:
            case ShaderDataType::Half4:
            case ShaderDataType::Int4:
            case ShaderDataType::UInt4:
            case ShaderDataType::Byte4:
            case ShaderDataType::UByte4:
            case ShaderDataType::Short4:
            case ShaderDataType::UShort4:
            case ShaderDataType::Double4:
            case ShaderDataType::DMat4:
                return 4;
        }

        return 0;
    }

    // 普通标量/向量占一个 location；NxN 矩阵占 N 个连续 location。
    [[nodiscard]] constexpr uint32_t ShaderDataTypeLocationCount(const ShaderDataType type) noexcept
    {
        switch (type)
        {
            case ShaderDataType::None: return 0;
            case ShaderDataType::FMat2:
            case ShaderDataType::DMat2: return 2;
            case ShaderDataType::FMat3:
            case ShaderDataType::DMat3: return 3;
            case ShaderDataType::FMat4:
            case ShaderDataType::DMat4: return 4;
            default: return 1;
        }
    }

    [[nodiscard]] constexpr bool ShaderDataTypeIsMatrix(const ShaderDataType type) noexcept
    {
        switch (type)
        {
            case ShaderDataType::FMat2:
            case ShaderDataType::FMat3:
            case ShaderDataType::FMat4:
            case ShaderDataType::DMat2:
            case ShaderDataType::DMat3:
            case ShaderDataType::DMat4:
                return true;
            default:
                return false;
        }
    }

    [[nodiscard]] constexpr bool ShaderDataTypeIsInteger(const ShaderDataType type) noexcept
    {
        switch (type)
        {
            case ShaderDataType::Int:
            case ShaderDataType::Int2:
            case ShaderDataType::Int3:
            case ShaderDataType::Int4:
            case ShaderDataType::UInt:
            case ShaderDataType::UInt2:
            case ShaderDataType::UInt3:
            case ShaderDataType::UInt4:
            case ShaderDataType::Byte:
            case ShaderDataType::Byte2:
            case ShaderDataType::Byte3:
            case ShaderDataType::Byte4:
            case ShaderDataType::UByte:
            case ShaderDataType::UByte2:
            case ShaderDataType::UByte3:
            case ShaderDataType::UByte4:
            case ShaderDataType::Short:
            case ShaderDataType::Short2:
            case ShaderDataType::Short3:
            case ShaderDataType::Short4:
            case ShaderDataType::UShort:
            case ShaderDataType::UShort2:
            case ShaderDataType::UShort3:
            case ShaderDataType::UShort4:
                return true;
            default:
                return false;
        }
    }

    [[nodiscard]] constexpr bool ShaderDataTypeIsDouble(const ShaderDataType type) noexcept
    {
        switch (type)
        {
            case ShaderDataType::Double:
            case ShaderDataType::Double2:
            case ShaderDataType::Double3:
            case ShaderDataType::Double4:
            case ShaderDataType::DMat2:
            case ShaderDataType::DMat3:
            case ShaderDataType::DMat4:
                return true;
            default:
                return false;
        }
    }

    [[nodiscard]] constexpr bool ShaderDataTypeIsValidVertexAttribute(const ShaderDataType type) noexcept
    {
        return ShaderDataTypeSize(type) != 0 && type != ShaderDataType::Bool;
    }

    /**
     * 像VBO中的 position normal uv等信息的存放
     */
    struct BufferElement
    {
        std::string Name;
        uint32_t Offset = 0;
        uint32_t Size = 0;
        ShaderDataType Type = ShaderDataType::None;
        // 对整数存储启用后，OpenGL 会将其归一化后作为浮点数据交给 Shader。
        bool Normalized = false;

        BufferElement() = default;

        BufferElement(const ShaderDataType type, std::string name, const bool normalized = false)
            : Name(std::move(name)), Size(ShaderDataTypeSize(type)), Type(type), Normalized(normalized)
        {
        }

        [[nodiscard]] uint32_t GetComponentSize() const noexcept
        {
            return ShaderDataTypeComponentCount(Type);
        }

        [[nodiscard]] uint32_t GetLocationCount() const noexcept
        {
            return ShaderDataTypeLocationCount(Type);
        }

        // 矩阵的一列所占字节数；标量和向量则返回元素自身大小。
        [[nodiscard]] uint32_t GetAttributeSize() const noexcept
        {
            const uint32_t locationCount = GetLocationCount();
            return locationCount == 0 ? 0 : Size / locationCount;
        }
    };

    class VertexBufferLayout
    {
    public:
        VertexBufferLayout() = default;

        VertexBufferLayout(const std::initializer_list<BufferElement> elements)
            : m_Elements(elements)
        {
            CalculateOffsetAndStride();
        }

        [[nodiscard]] const std::vector<BufferElement> &GetElements() const noexcept
        {
            return m_Elements;
        }

        [[nodiscard]] uint32_t GetStride() const noexcept
        {
            return m_Stride;
        }

        [[nodiscard]] auto begin() noexcept { return m_Elements.begin(); }
        [[nodiscard]] auto end() noexcept { return m_Elements.end(); }
        [[nodiscard]] auto begin() const noexcept { return m_Elements.cbegin(); }
        [[nodiscard]] auto end() const noexcept { return m_Elements.cend(); }

    private:
        void CalculateOffsetAndStride() noexcept
        {
            uint32_t offset = 0;
            for (auto &element : m_Elements)
            {
                element.Offset = offset;
                offset += element.Size;
            }
            m_Stride = offset;
        }

        std::vector<BufferElement> m_Elements;
        uint32_t m_Stride = 0;
    };

    class VertexBuffer
    {
    public:
        virtual ~VertexBuffer() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        virtual void SetLayout(const VertexBufferLayout &layout) = 0;
        [[nodiscard]] virtual const VertexBufferLayout &GetLayout() const = 0;

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

        static VertexBuffer *Create(const void *vertices, uint32_t size);
    };

}
