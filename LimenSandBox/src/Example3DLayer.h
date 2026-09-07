//
// Created by chenlong on 2026/8/20.
//
#pragma once
#include "Limen/Application/Layer.h"
#include "Limen/Renderer/Material.h"
#include "Limen/Renderer/Mesh.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "Limen/Renderer/RenderPass.h"
#include "Limen/RHI/Framebuffer.h"
#include "Limen/RHI/Shader.h"

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

        //Mesh
        Limen::Scope<Limen::Mesh> m_CubeMesh;

        /**
         * @brief 立方体绘制使用的材质。
         *
         * Material组合Pipeline、Shader普通参数和纹理绑定；
         * Mesh仍然只负责几何数据。
         */
        Limen::Ref<Limen::Material> m_CubeMaterial;

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
