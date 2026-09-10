//
// Created by chenlong on 2026/9/7.
//

#include <cmath>

#include <glm/geometric.hpp>
#include <glm/gtc/matrix_transform.hpp>

#include "Limen/Scene/Light.h"

#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneRenderer.h"

#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/RHI/Framebuffer.h"
#include "Limen/Renderer/RenderPass.h"

namespace Limen
{
    SceneRenderer::SceneRenderer(const SceneRendererSpecification &spec)
        : m_Spec(spec)
    {
        /*
         * 创建 Framebuffer 时宽、高和采样数都必须有效。
         *
         * Scene Viewport 被最小化产生的 0 × 0 尺寸，应当交给
         * Resize() 忽略，而不是用来创建初始 Framebuffer。
         */
        LM_CORE_ASSERT(
            m_Spec.Width > 0,
            "SceneRenderer width must be greater than zero"
        );

        LM_CORE_ASSERT(
            m_Spec.Height > 0,
            "SceneRenderer height must be greater than zero"
        );

        LM_CORE_ASSERT(
            m_Spec.Samples > 0,
            "SceneRenderer sample count must be at least one"
        );
        LM_CORE_ASSERT(
            m_Spec.ShadowMapResolution > 0,
            "SceneRenderer shadow map resolution must be greater than zero"
        );

        if (m_Spec.Width == 0 || m_Spec.Height == 0 || m_Spec.Samples == 0 || m_Spec.ShadowMapResolution == 0)
            return;

        /**
         * 第一步：创建这个份 SceneRenderer 独占的场景 FBO
         *
         * FBO 决定场景颜色、深度和模版结果存储在哪里
         */
        FramebufferSpecification framebufferSpec;

        framebufferSpec.Width = m_Spec.Width;
        framebufferSpec.Height = m_Spec.Height;
        framebufferSpec.Samples = m_Spec.Samples;

        m_Framebuffer = Framebuffer::Create(framebufferSpec);

        LM_CORE_ASSERT(m_Framebuffer, "SceneRenderer '{}' failed to create Framebuffer", m_Spec.DebugName);

        if (!m_Framebuffer)
            return;

        /*
         * 创建平行光Shadow Map使用的Depth-Only Framebuffer。
         *
         * Shadow Map只记录光源视角下的最近深度：
         * - 不需要颜色附件；
         * - 不使用MSAA；
         * - 深度必须是可供Shader采样的Texture2D。
         */
        FramebufferSpecification shadowFramebufferSpec;

        shadowFramebufferSpec.Width = m_Spec.ShadowMapResolution;
        shadowFramebufferSpec.Height = m_Spec.ShadowMapResolution;
        shadowFramebufferSpec.Samples = 1;

        shadowFramebufferSpec.Attachments = {FramebufferAttachmentFormat::Depth32F};

        m_ShadowFramebuffer = Framebuffer::Create(shadowFramebufferSpec);

        LM_CORE_ASSERT(m_ShadowFramebuffer,
                       "SceneRenderer '{}' failed to create shadow Framebuffer",
                       m_Spec.DebugName
        );

        if (!m_ShadowFramebuffer)
            return;

        /*
         * 描述Shadow Map的深度渲染阶段。
         *
         * 这个Pass没有颜色附件，只负责生成并保留光源视角深度。
         */
        RenderPassSpecification shadowRenderPassSpec;

        shadowRenderPassSpec.TargetFramebuffer = m_ShadowFramebuffer.get();
        /*
         * Shadow Framebuffer没有颜色附件，
         * 所以不需要读取、清理或者保存颜色。
         */
        shadowRenderPassSpec.ColorLoadOperation =
                AttachmentLoadOperation::DontCare;

        shadowRenderPassSpec.ColorStoreOperation =
                AttachmentStoreOperation::DontCare;

        // 每帧都要重新生成 Shadow Map, 因此开始时清楚上一帧留下的深度
        shadowRenderPassSpec.DepthLoadOperation = AttachmentLoadOperation::Clear;

        // 主场景 Shader随后需要采样该深度纹理 因此 Pass 结束后必须保留深度
        shadowRenderPassSpec.DepthStoreOperation = AttachmentStoreOperation::Store;

        /*
         * Depth32F没有模板分量，
         * 所以模板附件不参与这个Pass。
         */
        shadowRenderPassSpec.StencilLoadOperation = AttachmentLoadOperation::DontCare;
        shadowRenderPassSpec.StencilStoreOperation = AttachmentStoreOperation::DontCare;

        shadowRenderPassSpec.DebugName = m_Spec.DebugName + " Shadow RenderPass";

        m_ShadowRenderPass = CreateScope<RenderPass>(shadowRenderPassSpec);

        ShaderLibrary shaderLibrary;

        m_ShadowShader = shaderLibrary.Load("Renderer3D/ShadowDepth");

        LM_CORE_ASSERT(
            m_ShadowShader,
            "SceneRenderer '{}' failed to load ShadowDepth shader",
            m_Spec.DebugName
        );

        if (!m_ShadowShader)
            return;

        // Specification of Shadow Pipeline
        GraphicsPipelineSpecification shadowPPSpec;

        shadowPPSpec.ShaderProgram = m_ShadowShader;

        // Use triangles to build Mesh
        shadowPPSpec.Topology = PrimitiveTopology::TriangleList;

        // The purpose of "Shadow Map" is to obtain the depth closest to the light source
        shadowPPSpec.DepthTestEnabled = true;
        shadowPPSpec.DepthWriteEnabled = true;
        shadowPPSpec.DepthCompare = CompareOperation::Less;

        // Shadow Pass don't have color attachment, don't need to blend
        shadowPPSpec.Blend = BlendMode::Opaque;

        // 第一版继续剔除背面。
        // 后面处理 Shadow Acne 时，再讨论是否切换为正面剔除和添加 Depth Bias。
        shadowPPSpec.Culling = CullMode::Back;

        shadowPPSpec.FrontFaceWinding = FrontFace::CounterClockwise;

        shadowPPSpec.DebugName = m_Spec.DebugName + " Shadow Pipeline";

        /*
        * 第二步：描述主场景 RenderPass。
        *
        * 每帧重新绘制完整场景：
        * - 开始时清理颜色；
        * - 结束时保留颜色并进行必要的 MSAA Resolve；
        * - 开始时清理深度和模板；
        * - 结束后不再依赖深度模板内容。
        */
        RenderPassSpecification renderPassSpec;
        renderPassSpec.TargetFramebuffer = m_Framebuffer.get();
        renderPassSpec.ColorLoadOperation = AttachmentLoadOperation::Clear;
        renderPassSpec.ColorStoreOperation = AttachmentStoreOperation::Store;
        renderPassSpec.ClearColor = m_Spec.ClearColor;
        renderPassSpec.DebugName = m_Spec.DebugName + " Main RenderPass";

        /*
         * RenderPass 不拥有 Framebuffer，只保存它的非拥有指针。
         * m_RenderPass 在成员声明中位于 m_Framebuffer 后面，
         * 因此析构时会先销毁 RenderPass，再销毁 Framebuffer。
         */
        m_RenderPass = CreateScope<RenderPass>(renderPassSpec);

        m_ShadowPipeline = GraphicsPipeline::Create(shadowPPSpec);

        LM_CORE_ASSERT(
            m_ShadowPipeline,
            "SceneRenderer '{}' failed to create Shadow Pipeline",
            m_Spec.DebugName
        );

        if (!m_ShadowPipeline)
            return;
    }

    void SceneRenderer::RecalculateDirectionalLightViewProjection(const DirectionalLight &directionalLight)
    {
        /*
         * Direction约定为光从光源射向场景的传播方向。
         *
         * 必须先归一化，否则后面乘LightDistance时，
         * 光源距离会受到Direction长度影响。
         */
        constexpr float minDirectionLengthSquared = 1e-6f;

        const float directionLengthSquared = glm::dot(directionalLight.Direction, directionalLight.Direction);

        LM_CORE_ASSERT(
            directionLengthSquared > minDirectionLengthSquared,
            "Directional light direction must not be zero"
        );


        if (directionLengthSquared <= minDirectionLengthSquared)
        {
            // 防止保留之前计算出的无效光源矩阵。
            m_DirectionalLightViewProjectionMatrix = glm::mat4(1.0f);
            return;
        }

        const glm::vec3 lightDirection = glm::normalize(directionalLight.Direction);

        /*
         * 第一版让Shadow Map覆盖世界原点附近的场景。
         *
         * 当前两个立方体都在原点附近，因此先使用原点。
         * 后面实现稳定阴影和CSM时，再改成跟随相机视锥。
         */
        constexpr glm::vec3 focusPoint{0.0f, 0.0f, 0.0f};

        /*
        * 平行光本身没有真实位置。
        *
        * 但glm::lookAt需要一个观察位置，所以沿光线传播方向
        * 的反方向，构造一个虚拟光源相机位置。
        */
        constexpr float lightDistance = 15.0f;

        const glm::vec3 lightPosition = focusPoint - lightDirection * lightDistance;

        /*
         * lookAt需要一个Up方向。
         *
         * 如果光线方向几乎与世界Y轴平行，
         * 两个方向的叉积接近零，观察矩阵会失效。
         * 此时改用世界Z轴作为Up。
         */
        glm::vec3 lightUp{0.0f, 1.0f, 0.0f};

        if (std::abs(glm::dot(lightDirection, lightUp)) > 0.99f) // l // up
        {
            lightUp = glm::vec3(0.0f, 0.0f, 1.0f);
        }

        /*
         * 构造世界空间到光源观察空间的View Matrix。
         *
         * glm::lookAt内部完成的正是刚才推导的过程：
         * 1. 计算g、r、t；
         * 2. 构造世界到观察空间的旋转；
         * 3. 将lightPosition移动到观察空间原点；
         * 4. 得到View = R × T。
         */
        const glm::mat4 lightView = glm::lookAt(lightPosition, focusPoint, lightUp);

        /*
         * 平行光没有透视近大远小，因此使用正交投影。
         *
         * 当前第一版覆盖光源观察空间中：
         * x ∈ [-10, 10]
         * y ∈ [-10, 10]
         * 深度距离 ∈ [0.1, 50]
         */
        constexpr float shadowHalfExtent = 10.0f;
        constexpr float shadowNearPlane = 0.1f;
        constexpr float shadowFarPlane = 50.0f;
        const glm::mat4 lightProj = glm::ortho(
            -shadowHalfExtent,
            shadowHalfExtent,
            -shadowHalfExtent,
            shadowHalfExtent,
            shadowNearPlane,
            shadowFarPlane
        );

        m_DirectionalLightViewProjectionMatrix = lightProj * lightView;
    }

    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Render(const Scene &scene, const Camera &camera)
    {
        LM_CORE_ASSERT(m_RenderPass, "SceneRenderer '{}' has no RenderPass", m_Spec.DebugName);

        if (!m_RenderPass)
            return;

        // 不允许同一个 RendererPass 重复开始
        if (m_RenderPass->IsActive())
        {
            LM_CORE_ASSERT(
                false,
                "SceneRenderer '{}' RenderPass is already active",
                m_Spec.DebugName
            );
            return;
        }

        /*
        * 平行光方向可能在运行时被编辑，
        * 因此当前第一版每帧重新计算光源View-Projection矩阵。
        *
        * 后续可以使用Dirty Flag，只在光源或阴影范围变化时重算。
        */
        RecalculateDirectionalLightViewProjection(scene.GetDirectionalLight());

        LM_CORE_ASSERT(
            m_ShadowRenderPass,
            "SceneRenderer '{}' has no Shadow RenderPass",
            m_Spec.DebugName
        );

        if (!m_ShadowRenderPass)
            return;

        // 绑定 Shadow Framebuffer，并清除上一帧深度。
        m_ShadowRenderPass->Begin();

        if (!m_ShadowRenderPass->IsActive())
            return;

        LM_CORE_ASSERT(
            m_ShadowPipeline,
            "SceneRenderer '{}' has no Shadow Pipeline",
            m_Spec.DebugName
        );

        if (!m_ShadowPipeline)
        {
            m_ShadowRenderPass->End();
            return;
        }

        /*
         * 遍历场景中的全部可渲染物体。
         *
         * Shadow Pass只需要Mesh和Transform，
         * 不需要使用物体的Material。
         */
        for (const SceneRenderObject &renderObject: scene.GetRenderObjects())
        {
            LM_CORE_ASSERT(
                renderObject.MeshResource,
                "Shadow Pass encountered an object without Mesh"
            );

            if (!renderObject.MeshResource)
                continue;

            Renderer::SubmitDepth(
                *m_ShadowPipeline,
                *renderObject.MeshResource,
                m_DirectionalLightViewProjectionMatrix,
                renderObject.Transform
            );
        }


        m_ShadowRenderPass->End();

        // 开始物理渲染阶段
        m_RenderPass->Begin();

        // begin 失败
        if (!m_RenderPass->IsActive())
            return;

        constexpr uint32_t shadowMapTextureSlot = 1;

        m_ShadowFramebuffer->BindDepthAttachment(shadowMapTextureSlot);

        // 把相机、光源和阴影数据交给Renderer。
        Renderer::BeginScene(
            camera,
            scene.GetAmbientLight(),
            scene.GetDirectionalLight(),
            scene.GetPointLights(),
            m_DirectionalLightViewProjectionMatrix,
            shadowMapTextureSlot
        );

        /*
         * Scene 只保存对象数据，SceneRenderer 负责遍历并提交。
         *
         * 使用 const 引用，避免复制 SceneRenderObject：
         * - 不增加 Mesh 和 Material 的 Ref 引用计数；
         * - 不复制 Transform 矩阵。
         */
        for (const auto &[MeshResource,MaterialResource,Transform]
             : scene.GetRenderObjects())
        {
            /*
             * AddRenderObject() 已经禁止加入无效对象。
             * 这里仍保留保护，避免未来反序列化或其他入口产生空资源。
             */
            LM_CORE_ASSERT(
                MeshResource,
                "SceneRenderer '{}' encountered an object without Mesh",
                m_Spec.DebugName
            );

            LM_CORE_ASSERT(
                MaterialResource,
                "SceneRenderer '{}' encountered an object without Material",
                m_Spec.DebugName
            );

            if (!MeshResource || !MaterialResource)
            {
                continue;
            }

            /*
             * Material：提供 Shader、Pipeline、纹理和材质参数；
             * Mesh：提供 VAO、VBO 和 IBO；
             * Transform：描述该物体在世界中的位置、旋转和缩放。
             */
            Renderer::Submit(
                *MaterialResource,
                *MeshResource,
                Transform);
        }


        /**
         * 结束逻辑场景
         */
        Renderer::EndScene();

        //结束物理渲染场景 完成 MSAA Resolve，解绑FBO
        m_RenderPass->End();
    }

    void SceneRenderer::Resize(const uint32_t width, const uint32_t height)
    {
        // ImGui 面板折叠或缩到最小时可能得到 0 × 0。
        // OpenGL 不能创建这种尺寸的 Framebuffer 附件，因此直接忽略。
        if (width == 0 || height == 0)
            return;

        // 尺寸没有变化时，不重新申请 GPU 资源。
        if (width == m_Spec.Width && height == m_Spec.Height)
            return;

        LM_CORE_ASSERT(m_Framebuffer, "SceneRenderer '{}' has no Framebuffer", m_Spec.DebugName);

        if (!m_Framebuffer)
            return;

        //重新创建 FBO 对应的尺寸
        m_Framebuffer->Resize(width, height);

        // 同步保存 SceneRenderer 当前使用的尺寸
        m_Spec.Width = width;
        m_Spec.Height = height;
    }

    std::uintptr_t SceneRenderer::GetFinalColorAttachmentHandle() const noexcept
    {
        // Forbidding nullptr
        if (!m_Framebuffer)
            return 0;
        return m_Framebuffer->GetColorAttachmentHandle();
    }
}
