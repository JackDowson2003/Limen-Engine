//
// Created by chenlong on 2026/8/26.
//
#pragma once


#include "Limen/Application/Layer.h"
#include "Limen/Core/DeltaTime.h"
#include "Limen/Events/Event.h"
#include "Limen/Renderer/OrthoGraphicCameraController.h"
#include "Limen/Renderer/RenderPass.h"
#include "Limen/RHI/Texture.h"
#include "Limen/RHI/Framebuffer.h"


namespace SandBox
{
    class Renderer2DTestLayer final : public Limen::Layer
    {
    public:
        Renderer2DTestLayer();

        ~Renderer2DTestLayer() override = default;

        /**
         * @brief 创建测试层独占的 Framebuffer、RenderPass 和纹理资源。
         */
        void OnAttach() override;

        /**
         * @brief 在 Layer 从 LayerStack 移除时主动释放测试资源。
         */
        void OnDetach() override;

        /**
         * @brief 更新相机并把二维场景渲染到离屏 Framebuffer。
         */
        void OnUpdate(Limen::DeltaTime &deltaTime) override;

        /**
         * @brief 在 ImGui Scene 面板中显示 Framebuffer 颜色附件。
         */
        void OnImGuiRender() override;

        /**
         * @brief 将输入事件传递给正交相机控制器。
         */
        void OnEvent(Limen::Event &event) override;

    private:

        /**
        * @brief 控制二维场景使用的正交相机。
        */
        Limen::OrthoGraphicCameraController m_CameraController;

        /**
         * @brief 保存 Renderer2D 绘制结果的离屏 Framebuffer。
         *
         * Framebuffer 必须声明在 RenderPass 前面，因为成员按声明的
         * 逆序销毁，从而保证 RenderPass 先销毁。
         */
        Limen::Scope<Limen::Framebuffer> m_SceneFramebuffer;

        /**
         * @brief 管理场景 Framebuffer 的绑定、清理、解绑和 MSAA Resolve。
         *
         * RenderPass 只借用 m_SceneFramebuffer。
         */
        Limen::Scope<Limen::RenderPass> m_SceneRenderPass;

        /**
         * @brief 用于验证 Renderer2D 纹理绘制接口的测试纹理。
         */
        Limen::Ref<Limen::Texture2D> m_CheckerboardTexture;

        /**
         * @brief ImGui Scene 内容区域希望使用的像素尺寸。
         */
        uint32_t m_ViewportWidth = 1280;
        uint32_t m_ViewportHeight = 720;
    };
}
