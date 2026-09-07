//
// Created by chenlong on 2026/9/7.
//

#pragma once

#include <cstdint>
#include <string>

#include <glm/vec4.hpp>
#include "Limen/Core/Core.h"

namespace Limen
{
    class Camera;
    class Scene;
    class Framebuffer; //FBO
    class RenderPass; //负责渲染的开始和结束

    /**
     * @brief 创建 SceneRenderer 所需的配置。
     *
     * SceneRenderer 会根据这些参数创建自己的 Framebuffer
     * 和主场景 RenderPass。
     */
    struct SceneRendererSpecification
    {
        // 场景渲染目标的像素宽度
        uint32_t Width = 1;

        // 场景渲染目标的像素高度
        uint32_t Height = 1;

        // MSAA 采样数，1 表示关闭MSAA
        uint32_t Samples = 4;

        // 每帧开始时用于清理颜色附件的颜色。
        glm::vec4 ClearColor{
            0.1f,
            0.1f,
            0.1f,
            1.0f
        };

        // 用于日志、调试器和未来 GPU 标记的名称。
        std::string DebugName = "Scene Renderer";
    };

    /**
     * @brief 把 Scene 中的可渲染对象绘制到独立 Framebuffer。
     *
     * SceneRenderer 负责组织：
     *
     * 1. RenderPass::Begin()；
     * 2. Renderer::BeginScene()；
     * 3. 遍历 SceneRenderObject；
     * 4. Renderer::Submit()；
     * 5. Renderer::EndScene()；
     * 6. RenderPass::End()。
     *
     * SceneRenderer 不拥有 Scene 和 Camera，它只在 Render()
     * 调用期间读取它们。
     */
    class LIMEN_API SceneRenderer final
    {
    public:
        explicit SceneRenderer(const SceneRendererSpecification& spec);

        /**
        * 必须在 SceneRenderer.cpp 中定义。
        *
        * Framebuffer 和 RenderPass 在头文件中只有前置声明，
        * Scope 析构时需要看到它们的完整类型。
        */
        ~SceneRenderer();

        SceneRenderer(const SceneRenderer&) = delete;
        SceneRenderer& operator=(const SceneRenderer&) = delete;

        /**
         * @brief 使用指定 Camera 渲染 Scene。
         *
         * @param scene 保存 Mesh、Material 和 Transform 的场景。
         * @param camera 本次渲染使用的相机。
         */
        void Render(const Scene& scene, const Camera& camera);

        /**
         * @brief 修改内部 Framebuffer 的像素尺寸。
         *
         * Scene Viewport 大小变化时调用。
         */
        void Resize(
            uint32_t width,
            uint32_t height
        );

        /**
         * @brief 获取最终可采样颜色纹理的后端句柄。
         *
         * 当前 OpenGL 后端返回完成 MSAA Resolve 后的 Texture ID，
         * 可以交给 ImGui::Image() 显示。
         */
        [[nodiscard]]
        std::uintptr_t
        GetFinalColorAttachmentHandle() const noexcept;

        /**
         * @brief 获取 SceneRenderer 当前使用的规格。
         *
         * 返回 const 引用：
         * 1. 不复制包含 std::string 的 Specification；
         * 2. 外部只能读取，不能绕过 SceneRenderer 修改尺寸；
         * 3. Width 和 Height 会在 Resize() 成功后同步更新。
         */
        [[nodiscard]]
        const SceneRendererSpecification&
        GetSpecification() const noexcept
        {
            return m_Spec;
        }

    private:
        // 保存创建尺寸、MSAA采样数和清屏颜色
        SceneRendererSpecification m_Spec;

        /**
         * SceneRenderer 独占场景渲染目标
         *
         * 必须声明在 m_RendererPass 前，这样del时 RenderPass会先销毁,FBO后销毁
         */
        Scope<Framebuffer> m_Framebuffer;

        /**
         * 主场景渲染阶段。
         *
         * RenderPass 内部非拥有地引用 m_Framebuffer。
         */
        Scope<RenderPass> m_RenderPass;
    };
}