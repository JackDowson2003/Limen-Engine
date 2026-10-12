//
// Created by chenlong on 2026/8/26.
//
#include "Renderer2DTestLayer.h"
#include <cmath>
#include "imgui.h"
#include "Limen/Renderer/Renderer2D.h"
#include "Limen/Core/Log.h"
#include "Limen/Asset/AssetManager.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/Renderer/Renderer.h"

namespace SandBox
{
    Renderer2DTestLayer::Renderer2DTestLayer()
        : Layer("Renderer2D Test Layer"), m_CameraController(
              1600.0f / 900.0f,
              false
          )
    {
        // 粒子从世界坐标原点产生。
        m_ParticleSpecification.Position =
                glm::vec3(0.0f, 0.0f, 0.0f);

        //X Y Z三个方向上的速度的随机变化的范围
        m_ParticleSpecification.VelocityVariation =
                glm::vec3(.4f, .2f, .0f);

        // 粒子每秒向上移动0.6个世界单位。
        m_ParticleSpecification.Velocity =
                glm::vec3(0.0f, 0.6f, 0.0f);

        // 开始时为橙红色且完全不透明。
        m_ParticleSpecification.ColorBegin =
                glm::vec4(1.0f, 0.2f, 0.05f, 1.0f);

        // 结束时为黄色且完全透明。
        m_ParticleSpecification.ColorEnd =
                glm::vec4(1.0f, 1.0f, 0.1f, 0.0f);

        // 从较小尺寸逐渐缩小到0。
        m_ParticleSpecification.SizeBegin = 0.15f;
        m_ParticleSpecification.SizeEnd = 0.0f;

        // 每个粒子存活1.5秒。
        m_ParticleSpecification.LifeTime = 1.5f;

        //其余参数默认
    }

    void Renderer2DTestLayer::OnAttach()
    {
        Limen::FramebufferSpecification spec;
        spec.Width = m_ViewportWidth;
        spec.Height = m_ViewportHeight;
        spec.Samples = 4;
        m_SceneFramebuffer = Limen::Framebuffer::Create(spec);

        LM_CORE_ASSERT(
            m_SceneFramebuffer,
            "Renderer2D test failed to create scene Framebuffer"
        );

        if (!m_SceneFramebuffer)
            return;

        Limen::RenderPassSpecification renderPassSpec;
        renderPassSpec.ClearColor = {0.08f, 0.08f, 0.1f, 1.0f};
        renderPassSpec.TargetFramebuffer = m_SceneFramebuffer.get();
        renderPassSpec.DebugName = "Render Pass 2D";

        m_SceneRenderPass = Limen::CreateScope<Limen::RenderPass>(renderPassSpec);

        LM_CORE_ASSERT(
            m_SceneRenderPass,
            "Renderer2D test failed to create scene RenderPass"
        );

        if (!m_SceneRenderPass)
        {
            m_SceneFramebuffer.reset();
            return;
        }

        Limen::FramebufferSpecification displaySpec;
        displaySpec.Width = m_ViewportWidth;
        displaySpec.Height = m_ViewportHeight;
        displaySpec.Samples = 1;
        displaySpec.Attachments = {Limen::FramebufferAttachmentFormat::RGBA8};

        m_DisplayFramebuffer = Limen::Framebuffer::Create(displaySpec);

        LM_CORE_ASSERT(m_DisplayFramebuffer, "Renderer2D display Framebuffer creation failed");

        if (!m_DisplayFramebuffer)
        {
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }

        Limen::RenderPassSpecification displayPassSpec;
        displayPassSpec.TargetFramebuffer = m_DisplayFramebuffer.get();
        displayPassSpec.ColorLoadOperation = Limen::AttachmentLoadOperation::Clear;
        displayPassSpec.ColorStoreOperation = Limen::AttachmentStoreOperation::Store;
        displayPassSpec.DepthLoadOperation = Limen::AttachmentLoadOperation::DontCare;
        displayPassSpec.DepthStoreOperation = Limen::AttachmentStoreOperation::DontCare;
        displayPassSpec.StencilLoadOperation = Limen::AttachmentLoadOperation::DontCare;
        displayPassSpec.StencilStoreOperation = Limen::AttachmentStoreOperation::DontCare;
        displayPassSpec.ClearColor = {0.0f, 0.0f, 0.0f, 1.0f};
        displayPassSpec.DebugName = "Renderer2D Display RenderPass";

        m_DisplayRenderPass = Limen::CreateScope<Limen::RenderPass>(displayPassSpec);

        Limen::ShaderLibrary shaderLibrary;
        m_DisplayShader = shaderLibrary.Load("Renderer2D/DisplayEncode");

        LM_CORE_ASSERT(m_DisplayShader, "Renderer2D display Shader load failed");

        if (!m_DisplayShader)
        {
            m_DisplayRenderPass.reset();
            m_DisplayFramebuffer.reset();
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }

        m_DisplayShader->Bind();
        m_DisplayShader->SetInt("u_SceneColor", 0);
        m_DisplayShader->UnBind();

        Limen::GraphicsPipelineSpecification displayPipelineSpec;
        displayPipelineSpec.ShaderProgram = m_DisplayShader;
        displayPipelineSpec.Blend = Limen::BlendMode::Opaque;
        displayPipelineSpec.Topology = Limen::PrimitiveTopology::TriangleList;
        displayPipelineSpec.Culling = Limen::CullMode::None;
        displayPipelineSpec.DepthWriteEnabled = false;
        displayPipelineSpec.DepthTestEnabled = false;
        displayPipelineSpec.FrontFaceWinding = Limen::FrontFace::CounterClockwise;
        displayPipelineSpec.DebugName = "Renderer2D Display Pipeline";

        m_DisplayPipeline = Limen::GraphicsPipeline::Create(displayPipelineSpec);

        LM_CORE_ASSERT(m_DisplayPipeline, "Renderer2D display Pipeline creation failed");

        if (!m_DisplayPipeline)
        {
            m_DisplayShader.reset();
            m_DisplayRenderPass.reset();
            m_DisplayFramebuffer.reset();
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }

        m_DisplayVertexArray.reset(Limen::VertexArray::Create());

        LM_CORE_ASSERT(m_DisplayVertexArray, "Renderer2D display VertexArray creation failed");

        if (!m_DisplayVertexArray)
        {
            m_DisplayPipeline.reset();
            m_DisplayShader.reset();
            m_DisplayRenderPass.reset();
            m_DisplayFramebuffer.reset();
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }
        m_DisplayVertexArray->Bind();
        constexpr uint32_t fullscreenIndices[] = {0, 1, 2};
        Limen::Ref<Limen::IndexBuffer> indexBuffer;
        indexBuffer.reset(Limen::IndexBuffer::Create(fullscreenIndices, 3));

        LM_CORE_ASSERT(indexBuffer, "Renderer2D display IndexBuffer creation failed");
        if (!indexBuffer)
        {
            m_DisplayVertexArray->UnBind();
            m_DisplayVertexArray.reset();
            m_DisplayPipeline.reset();
            m_DisplayShader.reset();
            m_DisplayRenderPass.reset();
            m_DisplayFramebuffer.reset();
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }

        m_DisplayVertexArray->SetIndexBuffer(indexBuffer);
        m_DisplayVertexArray->UnBind();

        // 棋盘格是颜色纹理：GPU 采样时将 RGB 解码到 Linear；Alpha 保持原值。
        // Display Pass 在所有场景绘制完成后统一编码输出。
        m_CheckerboardTexture = Limen::AssetManager::LoadTexture2D(
            "textures/checkerboard.png",
            Limen::TextureColorSpace::SRGB
        );

        LM_CORE_ASSERT(
            m_CheckerboardTexture,
            "Renderer2D test failed to create Checkerboard Texture"
        );

        if (!m_CheckerboardTexture)
        {
            m_DisplayShader.reset();
            m_DisplayVertexArray.reset();
            m_DisplayPipeline.reset();
            m_DisplayRenderPass.reset();
            m_DisplayFramebuffer.reset();
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }
    }

    void Renderer2DTestLayer::OnDetach()
    {
        m_DisplayShader.reset();
        m_DisplayVertexArray.reset();
        m_DisplayPipeline.reset();
        m_CheckerboardTexture.reset();
        m_DisplayRenderPass.reset();
        m_DisplayFramebuffer.reset();
        m_SceneRenderPass.reset();
        m_SceneFramebuffer.reset();
    }

    void Renderer2DTestLayer::OnUpdate(Limen::DeltaTime &deltaTime)
    {
        if (!m_DisplayVertexArray ||
            !m_DisplayPipeline ||
            !m_DisplayShader ||
            !m_SceneFramebuffer ||
            !m_SceneRenderPass ||
            !m_DisplayFramebuffer ||
            !m_DisplayRenderPass ||
            !m_CheckerboardTexture
        )
        {
            return;
        }

        if (m_ReferenceViewEnabled)
        {
            constexpr uint32_t referenceWidth = 1280;
            constexpr uint32_t referenceHeight = 720;
            m_ViewportWidth = referenceWidth;
            m_ViewportHeight = referenceHeight;
        }

        //窗口发生变化时，重新创建FBO的附件
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0 &&
            (
                m_ViewportWidth != m_SceneFramebuffer->GetSpecification().Width ||
                m_ViewportHeight != m_SceneFramebuffer->GetSpecification().Height)
        )
        {
            //更新FBO参数
            m_SceneFramebuffer->Resize(
                m_ViewportWidth,
                m_ViewportHeight
            );
        }

        if (m_ViewportWidth > 0 && m_ViewportHeight > 0 &&
            (m_ViewportWidth != m_DisplayFramebuffer->GetSpecification().Width ||
             m_ViewportHeight != m_DisplayFramebuffer->GetSpecification().Height))
        {
            m_DisplayFramebuffer->Resize(m_ViewportWidth, m_ViewportHeight);
        }

        m_CameraController.OnResize(
            static_cast<float>(m_ViewportWidth),
            static_cast<float>(m_ViewportHeight)
        );

        if (!m_ReferenceViewEnabled)
            m_CameraController.OnUpdate(deltaTime);

        m_ParticleSystem.Update(deltaTime);


        if (m_ParticleEmissionRate > 0.f && m_MaxParticleEmissionsPerFrame > 0)
        {
            //每个粒子需要个多久发射一次
            const float emissionInterval = 1.f / m_ParticleEmissionRate;

            //累计经过本帧的时间
            m_ParticleEmissionTimeAccumulator += deltaTime.GetSeconds();

            //这帧发射了了几个
            uint32_t emissionsThisFrame = 0;


            /*
             * 累计时间达到一个发射间隔后产生粒子。
             *
             * 使用 while 是因为某一帧可能很慢（掉帧了），
             * 一帧积累的时间可能足够产生多个粒子。
             */
            while (m_ParticleEmissionTimeAccumulator >= emissionInterval && emissionsThisFrame <
                   m_MaxParticleEmissionsPerFrame)
            {
                m_ParticleSystem.Emit(m_ParticleSpecification);

                //消费一个时间间隔
                //这会导致掉帧的时候会出现突发
                m_ParticleEmissionTimeAccumulator -= emissionInterval;

                ++emissionsThisFrame;
            }
            //掉帧重补了四个 就重置一下m_ParticleEmissionTimeAccumulator
            if (
                emissionsThisFrame == m_MaxParticleEmissionsPerFrame
                && m_ParticleEmissionTimeAccumulator >= emissionInterval
            )
                m_ParticleEmissionTimeAccumulator = std::fmod(m_ParticleEmissionTimeAccumulator, emissionInterval);
        } else //下次重启就从0开始
        {
            m_ParticleEmissionTimeAccumulator = 0.f;
        }

        // 绑定并清理场景
        // 需要绑定FBO 以及清理一下上一帧的颜色
        m_SceneRenderPass->Begin();

        /*
         * Renderer2D 开始记录使用当前正交相机的二维场景。
         */
        Limen::Renderer2D::BeginScene(m_CameraController.GetCamera());

        // 参考模式只隐藏粒子绘制；模拟继续运行，切回普通模式仍保持正常时间推进。
        if (!m_ReferenceViewEnabled)
        {
            m_ParticleSystem.Render();
        }

        /*
         * 左侧纯色 Quad。
         */
        Limen::Renderer2D::DrawQuad(
            {-0.75f, 0.0f, 0.0f},
            {1.0f, 1.0f},
            {0.2f, 0.35f, 0.85f, 0.8f}
        );

        /*
         * 右侧纹理 Quad。
         */
        Limen::Renderer2D::DrawQuad(
            {0.75f, 0.0f, 0.0f},
            {1.0f, 1.0f},
            m_CheckerboardTexture
        );

        Limen::Renderer2D::EndScene();

        /*
         * 此时绘制完成了，开始进行合并
         * 解绑场景 Framebuffer，并执行 MSAA Resolve。
         */
        m_SceneRenderPass->End();

        if (m_DisplayRenderPass->IsActive())
        {
            LM_CORE_ASSERT(false, "Renderer2D display RenderPass is already active");
            return;
        }

        m_DisplayRenderPass->Begin();
        if (!m_DisplayRenderPass->IsActive())
            return;

        constexpr uint32_t sceneColorTextureSlot = 0;
        m_SceneFramebuffer->BindColorAttachment(sceneColorTextureSlot);

        Limen::Renderer::SubmitFullscreen(*m_DisplayPipeline, *m_DisplayVertexArray);
        m_DisplayRenderPass->End();
    }

    void Renderer2DTestLayer::OnImGuiRender()
    {
        //更新分辨率
        if (const bool sceneVisible = ImGui::Begin("Renderer2D Test Layer");
            sceneVisible && m_DisplayFramebuffer)
        {
            if (ImGui::Checkbox("Fixed 2D reference view", &m_ReferenceViewEnabled) &&
                m_ReferenceViewEnabled)
            {
                auto &camera = m_CameraController.GetCamera();
                camera.SetPosition(glm::vec3{0.0f, 0.0f, 0.0f});
                camera.SetRotation(0.0f);
                m_CameraController.SetZoomLevel(1.0f);
            }

            if (const ImVec2 &viewportSize = ImGui::GetContentRegionAvail();
                viewportSize.x > 0.0f && viewportSize.y > 0.0f)
            {
                if (!m_ReferenceViewEnabled)
                {
                    m_ViewportWidth = static_cast<uint32_t>(viewportSize.x);
                    m_ViewportHeight = static_cast<uint32_t>(viewportSize.y);
                }

                const ImTextureID textureID =
                        m_DisplayFramebuffer->GetColorAttachmentHandle();

                ImGui::Image(
                    ImTextureRef(textureID),
                    viewportSize,
                    ImVec2(0.0f, 1.0f),
                    ImVec2(1.0f, 0.0f)
                );
            }
        }
        ImGui::End();
    }

    void Renderer2DTestLayer::OnEvent(Limen::Event &event)
    {
        if (!m_ReferenceViewEnabled)
            m_CameraController.OnEvent(event);
    }
}
