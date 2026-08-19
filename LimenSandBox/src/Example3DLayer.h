//
// Created by chenlong on 2026/8/20.
//
#pragma once
#include "Layer.h"
#include "Renderer/PerspectiveCamera.h"
#include "Renderer/Shader.h"
#include "Renderer/VertexArray.h"


namespace SandBox
{
    /**
     * @brief Sandbox中的3D渲染测试层。
     *
     * 该类只负责验证：
     *
     * - PerspectiveCamera；
     * - 3D顶点和索引；
     * - 深度测试；
     * - Model/View/Projection变换；
     * - Renderer::BeginScene/Submit/EndScene。
     *
     * 它不属于LimenEngine核心代码，只是客户端测试代码。
     */
    class Example3DLayer final : public Limen::Layer
    {
    public:
        Example3DLayer();
        ~Example3DLayer() override = default;

        /**
         * @brief 每帧更新并渲染3D测试场景。
         *
         * @param deltaTime
         * 当前帧与上一帧之间经过的时间。
         * 单位为秒，可以用于实现与帧率无关的旋转。
         */
        void OnUpdate(Limen::DeltaTime& deltaTime) override;

    private:
        /**
         * 透视相机
         * 默认在(0, 0, 3)
         */
        Limen::PerspectiveCamera m_Camera;

        Limen::Scope<Limen::VertexArray> m_CubeVAO;

        // 保存立方体8个角点的位置。
        Limen::Ref<Limen::VertexBuffer> m_CubeVBO;

        // 保存立方体6个面、12个三角形的36个索引。
        Limen::Ref<Limen::IndexBuffer> m_CubeIBO;

        // 负责将立方体顶点变换到裁剪空间并输出调试颜色。
        Limen::Ref<Limen::Shader> m_CubeShader;

        // 当前立方体旋转角度，单位为度。
        float m_CubeRotationDegrees = 0.0f;

        // 立方体每秒旋转的角度，单位为度/秒。
        float m_CubeRotationSpeed = 45.0f;

    };
} // SandBox
