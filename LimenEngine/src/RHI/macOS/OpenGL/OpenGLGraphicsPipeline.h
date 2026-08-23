//
// Created by chenlong on 2026/8/23.
//

#pragma once

#include "Limen/RHI/GraphicsPipeline.h"

namespace Limen
{
    /**
     * @brief OpenGL 图形管线实现。
     *
     * OpenGL 没有与 Direct3D 12 PSO 完全对应的单一对象，因此该类保存
     * 公共规格，并在 Bind() 时把它转换为 OpenGL 状态机调用。
     */
    class OpenGLGraphicsPipeline final : public GraphicsPipeline
    {
    public:
        /**
         * @brief 创建一条 OpenGL 图形管线。
         *
         * @param specification Shader 与全部固定功能状态。
         */
        explicit OpenGLGraphicsPipeline(const GraphicsPipelineSpecification &specification);

        ~OpenGLGraphicsPipeline() override = default;

        /**
         * @brief 绑定 Shader，并应用全部 OpenGL 管线状态。
         */
        void Bind() const override;

        /**
         * @brief 获取这条 Pipeline 保存的规格。
         *
         * @return Pipeline 规格的只读引用。
         */
        [[nodiscard]]
        const GraphicsPipelineSpecification &
        GetSpecification() const noexcept override
        {
            return m_Specification;
        }

    private:
        /**
         * Pipeline 拥有一份规格副本。
         *
         * ShaderProgram 是 Ref，因此复制规格只会增加引用计数，不会复制
         * 真正的 GPU Shader 对象。
         */
        GraphicsPipelineSpecification m_Specification;
    };
}
