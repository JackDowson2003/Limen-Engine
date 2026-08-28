//
// Created by chenlong on 2026/8/20.
//
#pragma once
#include "Limen/Application/Layer.h"
#include "Limen/Renderer/Mesh.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "Limen/Renderer/RenderPass.h"
#include "Limen/RHI/Framebuffer.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/Texture.h"
#include "Limen/RHI/VertexArray.h"

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
        void OnUpdate(Limen::DeltaTime &deltaTime) override;

        void OnEvent(Limen::Event& event) override;

        void OnImGuiRender() override;

    private:

        /**
         * @brief 缓存本测试层加载的Shader。
         *
         * Ref声明本身只会产生空shared_ptr，因此必须同时创建实际对象，
         * 才能在构造函数中安全调用Load()。
         */
        Limen::Ref<Limen::ShaderLibrary> m_ShaderLib = Limen::CreateRef<Limen::ShaderLibrary>();

        Limen::Scope<Limen::Framebuffer> m_SceneFramebuffer;

        // RenderPass 非拥有地引用 Framebuffer；逆序析构会先销毁 RenderPass。
        Limen::Scope<Limen::RenderPass> m_SceneRenderPass;

        // 透视相机控制器，默认相机位置为 (0, 0, 3)。
        Limen::PerspectiveCameraController m_CameraController;

        Limen::Scope<Limen::VertexArray> m_CubeVAO;

        //Mesh
        Limen::Scope<Limen::Mesh> m_CubeMesh;

        // 保存立方体24条顶点记录，每条记录包含Position、Normal和TexCoord。
        Limen::Ref<Limen::VertexBuffer> m_CubeVBO;

        // 保存立方体6个面、12个三角形的36个索引。
        Limen::Ref<Limen::IndexBuffer> m_CubeIBO;

        // 负责将立方体顶点变换到裁剪空间并输出调试颜色。
        Limen::Ref<Limen::Shader> m_CubeShader;

        /**
         * @brief 立方体绘制使用的完整图形管线。
         *
         * 它组合Shader、深度测试、混合、剔除和图元拓扑状态。
         */
        // 声明在 Shader 之后，因此成员逆序析构时会先释放 Pipeline。
        Limen::Ref<Limen::GraphicsPipeline> m_CubePipeline;

        /**
         * @brief 立方体材质使用的Albedo纹理。
         *
         * Albedo描述物体表面的基础颜色。
         * Fragment Shader会采样它，并把采样结果作为Blinn-Phong中的k_d。
         *
         * Ref表示该纹理资源可以被多个物体或材质共享。
         */
        Limen::Ref<Limen::Texture2D> m_AlbedoTexture;

        // 当前立方体旋转角度，单位为度。
        float m_CubeRotationDegrees = 0.0f;

        // 立方体每秒旋转的角度，单位为度/秒。
        float m_CubeRotationSpeed = 45.0f;

        // ImGui Scene 面板希望使用的渲染尺寸。
        uint32_t m_ViewportWidth = 1280;
        uint32_t m_ViewportHeight = 720;

        // 鼠标上一帧是否位于 Scene 面板中。
        bool m_ViewportHovered = false;

        // Scene 面板是否拥有键盘焦点。
        bool m_ViewportFocused = false;

        // 右键导航是否已经从 Scene 面板中启动。
        bool m_ViewportNavigationActive = false;

    };
} // SandBox
