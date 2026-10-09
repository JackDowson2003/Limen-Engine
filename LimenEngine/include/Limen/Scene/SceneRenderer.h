//
// Created by chenlong on 2026/9/7.
//

#pragma once

#include <cstdint>
#include <string>

#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "Limen/Core/Core.h"

namespace Limen
{
    class VertexArray;
    class Camera;
    class Scene;
    class Framebuffer; //FBO
    class RenderPass; //负责渲染的开始和结束
    struct DirectionalLight;

    class Shader;
    class GraphicsPipeline;

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

        /**
         * 平行光Shadow Map的单边分辨率。
         *
         * Shadow Map通常使用正方形纹理。
         * 该尺寸独立于Scene Viewport，因为它描述的是
         * 从光源视角保存深度时使用的精度。
         */
        uint32_t ShadowMapResolution = 2048;

        // 用于日志、调试器和未来 GPU 标记的名称。
        std::string DebugName = "Scene Renderer";
    };

    /**
     * @brief 把 Scene 中的可渲染对象绘制到独立 Framebuffer。
     *
     * 拥有FBO RenderPass pipeline shader
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
         * @brief 设置后处理使用的线性曝光倍率
         *
         * 1.0 表示不缩放
         * 0.0 表示黑色
         * @return 成功接收新值时返回true
         */
        [[nodiscard]]
        bool SetExposure(float exposure) noexcept;

        [[nodiscard]]
        float GetExposure() const noexcept;

        /**
         * @brief 获取后处理后的最终颜色纹理句柄。
         *
         * Main Pass 的线性 HDR 颜色先经过后处理，再写入用于显示的 RGBA8 纹理。
         * 当前后处理先按曝光倍率缩放线性 HDR RGB，再进行基础 Reinhard Tone Mapping，
         * 最终输出的 Alpha 固定为 1.0，表示 Scene 视口图像不透明。
         * 此接口不提供保留透明背景的场景导出图像。
         *
         * 句柄由 SceneRenderer 拥有，调用者只借用；SceneRenderer 销毁后不可使用。
         * 当前 OpenGL 后端返回 Texture ID；资源不存在时返回 0。
         */
        [[nodiscard]]
        std::uintptr_t
        GetFinalColorAttachmentHandle() const noexcept;

        /**
         * @brief 获取平行光 Shadow Map 的深度纹理Handle
         *
         * 主要用于编辑器调试显示，不允许调用者修改或拥有该纹理
         * 当前 OpenGL 后端返回深度纹理的 Texture ID
         */
        [[nodiscard]]
        std::uintptr_t GetShadowMapHandle() const noexcept;

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
        /**
         * 根据平行光方向计算光源的view matrix和 ortho matrix
         *
         * 最终结果用于把世界坐标变换到光源裁剪空间(world -> local)
         * 是Shadow Mapping第一遍和第二遍共同使用的矩阵。
         */
        void RecalculateDirectionalLightViewProjection(
            const DirectionalLight& directionalLight
        );

    private:
        // 保存创建尺寸、MSAA采样数和清屏颜色
        SceneRendererSpecification m_Spec;

        // 当前视角的后处理曝光倍率，下一步才会传给Shader
        float m_Exposure = 1.0f;

        /**
         * 世界空间到平行光裁剪空间的变换矩阵。
         *
         * 等于：
         * LightProjection × LightView
         *
         * 每个物体还需要继续乘自己的Model矩阵。
         */
        glm::mat4 m_DirectionalLightViewProjectionMatrix{1.0f};

        /**
         * Shadow Pass 使用的深度Shader
         *
         * 它只把模型点点变换到光源的clip space
         * 不计算Material, texture and lighting color
         */
        Ref<Shader> m_ShadowShader;

        // 场景后处理使用的 Shader；当前仅加载验证，尚未用于绘制。
        Ref<Shader> m_PostProcessShader;

        /**
         * Graphics pipeline of Shadow pass
         *
         * It's responsible for specifying:
         * - use Shadow Shader
         * - open Depth Test
         * - open Depth Write
         * - close color mixed
         * - draw Mesh in Triangle
         */
        Ref<GraphicsPipeline> m_ShadowPipeline;

        // 后处理的绘制状态；当前尚未提交全屏绘制命令。
        Ref<GraphicsPipeline> m_PostProcessPipeline;

        // SceneRenderer 独占的全屏三角形VAO, VAO 会持有他的 IBO
        Scope<VertexArray> m_PostProcessVertexArray;

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

        // 后处理的但采样颜色输出，SceneRenderer 独占
        Scope<Framebuffer> m_PostProcessFramebuffer;

        // 非拥有地引用 m_PostProcessFramebuffer；析构时先于它释放
        Scope<RenderPass> m_PostProcessRenderPass;

        /**
         * 平行光阴影使用的Depth-Only Framebuffer。
         *
         * 它没有颜色附件，只拥有一张可供Shader采样的
         * Depth32F深度纹理。
         */
        Scope<Framebuffer> m_ShadowFramebuffer;

        /**
         * 管理Shadow Map深度渲染阶段。
         *
         * 它非拥有地引用m_ShadowFramebuffer：
         * Begin()负责绑定并清除旧深度；
         * End()负责结束Pass并保留生成的深度。
         *
         * 声明在m_ShadowFramebuffer后面，
         * 可以保证析构时先销毁RenderPass，再销毁Framebuffer。
         */
        Scope<RenderPass> m_ShadowRenderPass;


    };
}