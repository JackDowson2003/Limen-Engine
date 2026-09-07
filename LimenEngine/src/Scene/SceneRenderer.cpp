//
// Created by chenlong on 2026/9/7.
//
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneRenderer.h"

#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/RHI/Framebuffer.h"
#include "Limen/Renderer/RenderPass.h"

namespace Limen
{
    SceneRenderer::SceneRenderer(const SceneRendererSpecification &spec)
        :m_Spec(spec)
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

        if (m_Spec.Width == 0 ||m_Spec.Height == 0 ||m_Spec.Samples == 0)
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

        LM_CORE_ASSERT(m_Framebuffer,"SceneRenderer '{}' failed to create Framebuffer",m_Spec.DebugName);

        if (!m_Framebuffer)
            return;

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

    }

    SceneRenderer::~SceneRenderer() = default;

    void SceneRenderer::Render(const Scene &scene, const Camera &camera)
    {
        LM_CORE_ASSERT(m_RenderPass,"SceneRenderer '{}' has no RenderPass",m_Spec.DebugName);

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

        // 开始物理渲染阶段
        m_RenderPass->Begin();

        // begin 失败
        if (!m_RenderPass->IsActive())
            return;

        /*
         * 把Scene中的相机、主平行光和全部点光源，
         * 交给Renderer建立本次渲染的场景快照。
         */
        Renderer::BeginScene(
            camera,
            scene.GetDirectionalLight(),
            scene.GetPointLights()
        );

        /*
         * Scene 只保存对象数据，SceneRenderer 负责遍历并提交。
         *
         * 使用 const 引用，避免复制 SceneRenderObject：
         * - 不增加 Mesh 和 Material 的 Ref 引用计数；
         * - 不复制 Transform 矩阵。
         */
        for (const auto &[
            MeshResource,
            MaterialResource,
            Transform]
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

        LM_CORE_ASSERT(m_Framebuffer,"SceneRenderer '{}' has no Framebuffer",m_Spec.DebugName);

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
