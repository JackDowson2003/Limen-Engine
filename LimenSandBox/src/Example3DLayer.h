//
// Created by chenlong on 2026/8/20.
//
#pragma once
#include "Limen/Application/Layer.h"
#include "Limen/Renderer/Material.h"
#include "Limen/Renderer/Mesh.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "Limen/RHI/Shader.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneRenderer.h"

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

        // 透视相机控制器，默认相机位置为 (0, 0, 3)。
        Limen::PerspectiveCameraController m_CameraController;

        //Mesh
        Limen::Ref<Limen::Mesh> m_CubeMesh;

        /**
         * @brief 立方体绘制使用的材质。
         *
         * Material组合Pipeline、Shader普通参数和纹理绑定；
         * Mesh仍然只负责几何数据。
         */
        Limen::Ref<Limen::Material> m_CubeMaterial;

        /**
         * @brief 当前测试层的场景数据。
         *
         * Scene 保存场景中的 Mesh、Material 和 Transform，
         * 但它本身不会调用任何渲染接口。
         *
         * Scene 由 Example3DLayer 独占，不需要使用指针。
         */
        Limen::Scene m_Scene;

        /**
         * @brief 负责将 m_Scene 渲染到独立 Framebuffer。
         *
         * 使用 Scope 是因为：
         * 1. SceneRenderer 不允许复制；
         * 2. 它独占内部 Framebuffer 和 RenderPass；
         * 3. 生命周期完全属于当前 Layer；
         * 4. 需要等构造器中准备好规格后再创建。
         */
        Limen::Scope<Limen::SceneRenderer> m_SceneRenderer;

        /**
         * @brief m_Scene 中立方体对象的句柄。
         *
         * Layer 不保存 vector 元素的指针或引用，
         * 而是通过该句柄请求 Scene 修改立方体。
         */
        Limen::SceneRenderObjectHandle m_CubeObjectHandle;

        /**
         * @brief 测试点光源在m_Scene中的句柄。
         *
         * 构造器创建点光源后保存该句柄，
         * OnImGuiRender后续通过它修改对应点光源。
         *
         * 句柄本身不拥有光源，只记录光源在Scene中的位置。
         */
        Limen::ScenePointLightHandle m_PointLightHandle;

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
