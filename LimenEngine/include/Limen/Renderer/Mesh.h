//
// Created by chenlong on 2026/8/28.
//

#pragma once
#include <cstdint>
#include <vector>
#include "Limen/Core/Core.h"
#include "glm/vec3.hpp"
#include "glm/vec2.hpp"
namespace Limen
{
    /**
     * @brief  3D Mesh中一条顶点记录
     * 用于描述物体的几何形状
     *
     * 字段顺序与当前的 Example3D 的顶点布局一致
     * Position、Normal、TexCoord
     */
    struct MeshVertex
    {
        // 顶点在模型局部空间中的位置。
        glm::vec3 Position{0.f};

        // 顶点在模型局部空间中的法线。
        glm::vec3 Normal{0.f, 0.f, 1.f};

        // 顶点的二维纹理坐标。
        glm::vec2 TexCoord{0.f, 0.f};
    };

    /**
     * @brief 创建一个Mesh所需的CPU端的几何数据
     *
     * MeshData 只保存顶点和索引，还没有创建VAO VBO IBO
     */
    struct MeshData
    {
        // positions of geometry
        std::vector<MeshVertex> Vertices;

        // indices of geometry
        std::vector<uint32_t> Indices;
    };

    class VertexArray;

    /**
     * @brief 一份可以提交给 Renderer 绘制的静态几何数据。
     *
     * Mesh 没有 OpenGLMesh、DX12Mesh 等不同实现需要选择
     * 所以不需要使用工厂
     * 构造时会根据 MeshData 创建 VAO、VBO 和 IBO。
     * Mesh 不负责选择 Shader、设置 Pipeline，也不负责发出绘制命令。
     */
    class LIMEN_API Mesh
    {
    public:
        /**
         * @brief 根据 CPU 端的顶点和索引数据创建 GPU 几何资源。
         *
         * @param data 包含顶点数组和索引数组的 CPU 端数据。
         */
        explicit Mesh(const MeshData& data);


        /**
         * 必须在 Mesh.cpp 中定义。
         *
         * 因为这里只对 VertexArray 做了前向声明，
         * 析构函数需要在能够看到 VertexArray 完整定义的位置实现。
         */
        ~Mesh();

        /**
         * @brief 获得提交绘制时使用的 VAO。
         *
         * 返回引用而不是智能指针，是因为调用者只有使用权，
         * Mesh 仍然拥有这个 VAO。
         */
        [[nodiscard]]
        const VertexArray& GetVertexArray() const;

    private:
        // Mesh 独占其VAO, VAO内部会通过Ref 保持与VBO IBO的存活
        Scope<VertexArray> m_VertexArray;
    };
}
