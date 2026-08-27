//
// Created by chenlong on 2026/8/26.
//
#include "Renderer2DTestLayer.h"
#include <cmath>
#include "imgui.h"
#include "Limen/Renderer/Renderer2D.h"
#include "Limen/Core/Log.h"

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

        m_CheckerboardTexture = Limen::Texture2D::Create(
            "assets/textures/checkerboard.png"
        );

        LM_CORE_ASSERT(
            m_CheckerboardTexture,
            "Renderer2D test failed to create Checkerboard Texture"
        );

        if (!m_CheckerboardTexture)
        {
            m_SceneRenderPass.reset();
            m_SceneFramebuffer.reset();
            return;
        }
    }

    void Renderer2DTestLayer::OnDetach()
    {
        m_CheckerboardTexture.reset();
        m_SceneRenderPass.reset();
        m_SceneFramebuffer.reset();
    }

    void Renderer2DTestLayer::OnUpdate(Limen::DeltaTime &deltaTime)
    {
        if (!m_SceneFramebuffer ||
            !m_SceneRenderPass ||
            !m_CheckerboardTexture)
            return;

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

        m_CameraController.OnResize(
            static_cast<float>(m_ViewportWidth),
            static_cast<float>(m_ViewportHeight)
        );


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

        m_ParticleSystem.Render();

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
    }

    void Renderer2DTestLayer::OnImGuiRender()
    {
        //更新分辨率
        if (const bool sceneVisible = ImGui::Begin("Renderer2D Test Layer");
            sceneVisible && m_SceneRenderPass)
        {
            if (const ImVec2 &viewportSize = ImGui::GetContentRegionAvail();
                viewportSize.x > 0.0f && viewportSize.y > 0.0f)
            {
                m_ViewportWidth = static_cast<uint32_t>(viewportSize.x);
                m_ViewportHeight = static_cast<uint32_t>(viewportSize.y);

                const ImTextureID textureID =
                        m_SceneFramebuffer->GetColorAttachmentHandle();

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
        m_CameraController.OnEvent(event);
    }
}
