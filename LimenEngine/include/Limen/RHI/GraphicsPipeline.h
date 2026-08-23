//
// Created by chenlong on 2026/8/23.
//

#pragma once

#include <string>

#include "Limen/Core/Core.h"
#include "Limen/RHI/RenderState.h"
#include "Limen/RHI/Shader.h"

namespace Limen
{
    /**
     * @brief 描述图形渲染管线使用的 Shader 与固定功能状态。
     *
     * OpenGL 后端在 Bind() 时更新状态机；Direct3D 12 后端将根据
     * 同一份规格创建 Pipeline State Object（PSO）。
     */
    struct GraphicsPipelineSpecification
    {
        /**
         * @brief 当前管线使用的 Shader。
         *
         * Pipeline 保存共享引用，因为同一个 Shader 可以由多条 Pipeline
         * 或多个 Material 共同使用。
         */
        Ref<Shader> ShaderProgram;

        /**
         * @brief 输入顶点的图元组装方式。
         */
        PrimitiveTopology Topology = PrimitiveTopology::TriangleList;

        /**
         * @brief 是否开启深度测试。
         *
         * 3D 不透明物体通常开启，普通 2D 与 UI 通常关闭。
         */
        bool DepthTestEnabled = true;

        /**
         * @brief 是否允许通过测试的片元写入深度缓冲。
         *
         * 不透明物体通常开启，半透明物体通常关闭。
         */
        bool DepthWriteEnabled = true;

        /**
         * @brief 深度测试的比较方式。
         *
         * Less 表示新片元的深度更小时通过测试。
         */
        CompareOperation DepthCompare = CompareOperation::Less;

        /**
         * @brief 当前管线的颜色混合模式。
         */
        BlendMode Blend = BlendMode::Opaque;

        /**
         * @brief 当前管线的三角形面剔除方式。
         */
        CullMode Culling = CullMode::Back;

        /**
         * @brief 被认为是三角形正面的顶点绕序。
         */
        FrontFace FrontFaceWinding = FrontFace::CounterClockwise;

        /**
         * @brief 用于日志、调试器和未来 Pipeline 缓存的可读名称。
         *
         * 该名称不是底层图形 API 对象句柄。
         */
        std::string DebugName = "Unnamed Graphics Pipeline";
    };

    /**
     * @brief 后端无关的图形管线接口。
     *
     * Pipeline 不提供 UnBind()。渲染器通过绑定下一条 Pipeline 完成状态
     * 切换，这也与 Direct3D 12 的 PSO 使用方式一致。
     */
    class LIMEN_API GraphicsPipeline
    {
    public:
        virtual ~GraphicsPipeline() = default;

        /**
         * @brief 绑定 Shader，并应用这条 Pipeline 的固定功能状态。
         *
         * 图元拓扑由绘制命令读取规格并传给 RendererAPI，因为 OpenGL
         * 在 glDrawElements() 时选择图元类型。
         */
        virtual void Bind() const = 0;

        /**
         * @brief 获取创建这条 Pipeline 时使用的规格。
         *
         * @return Pipeline 规格的只读引用。
         */
        [[nodiscard]]
        virtual const GraphicsPipelineSpecification& GetSpecification() const noexcept = 0;

        /**
         * @brief 根据当前 RendererAPI 创建对应的后端 Pipeline。
         *
         * @param specification Shader 与全部固定功能状态。
         *
         * @return 创建成功时返回共享引用；配置无效或后端未实现时返回 nullptr。
         */
        [[nodiscard]]
        static Ref<GraphicsPipeline> Create(
            const GraphicsPipelineSpecification& specification
        );
    };
}
