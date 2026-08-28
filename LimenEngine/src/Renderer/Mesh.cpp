//
// Created by chenlong on 2026/8/28.
//
#include "Limen/Renderer/Mesh.h"
#include "Limen/RHI/VertexArray.h" //不保留会报错

#include "Limen/Core/Log.h"

namespace Limen
{
    Mesh::Mesh(const MeshData &data)
    {
        // 当前渲染流程使用索引绘制，因此顶点和索引都不能为空。
        LM_CORE_ASSERT(
            !data.Vertices.empty(),
            "Cannot create Mesh without vertices"
        );
        LM_CORE_ASSERT(
            !data.Indices.empty(),
            "Cannot create Mesh without Indices"
        );

        const auto indexSize = data.Vertices.size();

        //1. 每个索引都必须指向一个真实存在的顶点。
        for (const uint32_t index: data.Indices)
        {
            LM_CORE_ASSERT(
                index < indexSize,
                "Mesh index {} is out of range, vertex count is {}",
                index,
                indexSize
            );
        }

        //2. 初始化VAO Obj
        m_VertexArray.reset(VertexArray::Create());

        LM_ASSERT(
            m_VertexArray,
            "Mesh::Init() initialing m_VertexArray failed"
        );

        //2. 创建VBO 供VAO使用
        Ref<VertexBuffer> vertexBuffer;

        const uint32_t vertexBufferSize =
                static_cast<uint32_t>(
                    data.Vertices.size() * sizeof(MeshVertex)
                );
        vertexBuffer.reset(VertexBuffer::Create(
            data.Vertices.data(), vertexBufferSize
        ));

        const VertexBufferLayout vertexLayout{
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float3, "a_Normal"},
            {ShaderDataType::Float2, "a_TexCoord"}
        };
        vertexBuffer->SetLayout(vertexLayout);

        m_VertexArray->AddVertexBuffer(vertexBuffer);

        //3. 创建索引缓冲区。
        // IndexBuffer::Create() 的第二个参数是索引数量，不是字节数。
        Ref<IndexBuffer> indexBuffer;

        indexBuffer.reset(
            IndexBuffer::Create(
                data.Indices.data(),
                static_cast<uint32_t>(data.Indices.size())
            )
        );

        LM_CORE_ASSERT(
            indexBuffer,
            "Failed to create IndexBuffer for Mesh"
        );

        // 将 IBO 记录到当前 Mesh 的 VAO 中。
        m_VertexArray->SetIndexBuffer(indexBuffer);
    }

    Mesh::~Mesh()
    {
        LM_CORE_ASSERT(
            m_VertexArray,
            "Mesh does not contain a valid VertexArray"
        );

        if (!m_VertexArray)
            return;

        m_VertexArray.reset();
    }

    const VertexArray &Mesh::GetVertexArray() const
    {
        LM_CORE_ASSERT(
            m_VertexArray,
            "Mesh does not contain a valid VertexArray"
        );

        return *m_VertexArray;
    }
}
