//
// Created by chenlong on 2026/8/23.
//

#pragma once

#include <string>
#include <cstdint>

#include "glm/vec4.hpp"
#include "Limen/Core/Core.h"

namespace Limen
{
    class Framebuffer;

    /**
     * @brief RendererPass开始如何处理附件原有内容
     */
    enum class AttachmentLoadOperation : uint8_t
    {
        /**
         * 保留原有附件中的内容
         *
         *用于第二个Pass继续读取或叠加第一个Pass的渲染结果。
         */
        Load = 0,

        /**
         * 使用RenderPassSpecification中指定的清理值清除附件。
         */
        Clear,

        /**
         * 不关心附件原有内容。
         *
         * 后端可以直接丢弃旧数据；之后不能依赖附件原来的像素。
         */
        DontCare
    };

    /**
     * @brief RenderPass结束时是否需要保留附件内容
     */
    enum class AttachmentStoreOperation : uint8_t
    {
        /**
         * 保留本次Pass产生的结果。
         *
         * 后续Pass采样、ImGui显示或Present时必须使用Store。
         */
        Store = 0,

        /**
         * Pass结束后不再需要该附件内容，允许后端丢弃。
         */
        DontCare
    };

    /**
     * @brief 描述一个渲染阶段的目标、清屏值与调试名称。
     *
     *  Begin() 根据 LoadOperation 决定保留、清理或忽略附件原有内容；
     *  End() 根据 StoreOperation 决定是否保留颜色结果并执行 MSAA Resolve。
     *
     *  当前 OpenGL 4.1 后端无法显式丢弃附件，因此 DontCare 作为生命周期
     *  语义使用：调用者不能再依赖该附件内容，支持显式丢弃的后端可以进一步优化。
     */
    struct RenderPassSpecification
    {
        /**
         * @brief 当前渲染目标的非拥有指针。
         *
         * RenderPass 不销毁 Framebuffer，因此调用者必须保证 Framebuffer
         * 的生命周期覆盖 RenderPass。当前 Scene Viewport 独占 Framebuffer，
         * 所以不额外使用 Ref 共享所有权。
         */
        Framebuffer *TargetFramebuffer = nullptr;

        /**
         * @brief Pass开始时如何处理颜色附件。
         *
         * 当前Scene Pass每帧重新绘制整个画面，所以默认为Clear。
         */
        AttachmentLoadOperation ColorLoadOperation = AttachmentLoadOperation::Clear;

        /**
         * @brief Pass结束时是否保留颜色附件。
         *
         * Scene颜色需要交给ImGui::Image显示，所以默认为Store。
         */
        AttachmentStoreOperation ColorStoreOperation = AttachmentStoreOperation::Store;

        /**
         * Pass开始时如何处理深度附件。
         *
         * 主场景和Shadow Pass通常都需要先清除旧深度。
         */
        AttachmentLoadOperation DepthLoadOperation = AttachmentLoadOperation::Clear;

        /**
         * Pass结束后是否保留深度。
         *
         * 主场景完成后通常不再使用深度；
         * Shadow Pass则必须保留，供主场景Shader采样。
         */
        AttachmentStoreOperation DepthStoreOperation = AttachmentStoreOperation::DontCare;

        /**
         * Pass开始时如何处理模板附件。
         *
         * 主场景的Depth24Stencil8拥有模板；
         * Depth32F Shadow Map没有模板。
         */
        AttachmentLoadOperation StencilLoadOperation = AttachmentLoadOperation::Clear;

        /**
         * Pass结束后是否保留模板附件。
         */
        AttachmentStoreOperation StencilStoreOperation = AttachmentStoreOperation::DontCare;

        /** @brief Begin() 时用于清理颜色附件的颜色。 */
        glm::vec4 ClearColor{0.1f, 0.1f, 0.1f, 1.f};

        /** @brief 用于日志、调试器和未来 GPU Debug Marker 的名称。 */
        std::string DebugName = "Unnamed RenderPass";
    };

    /**
     * @brief 管理一个渲染阶段的开始与结束。
     *
     * Begin() 绑定目标 Framebuffer、设置清屏颜色并清除附件；End() 解绑
     * Framebuffer，并在启用 MSAA 时把多采样颜色解析到普通颜色纹理。
     *
     * RenderPass 是明确的作用域对象，不允许复制，避免同一个逻辑阶段被
     * 多个对象同时标记为活动状态。
     */
    class LIMEN_API RenderPass final
    {
    public:
        explicit RenderPass(const RenderPassSpecification &spec);

        ~RenderPass() = default;

        RenderPass(const RenderPass &) = delete;

        RenderPass &operator=(const RenderPass &) = delete;

        /** @brief 开始当前渲染阶段。 */
        void Begin();

        /** @brief 结束当前渲染阶段。 */
        void End();

        [[nodiscard]]
        const RenderPassSpecification &GetSpecification() const noexcept
        {
            return m_Specification;
        }

        [[nodiscard]]
        bool IsActive() const noexcept
        {
            return m_IsActive;
        }

    private:
        RenderPassSpecification m_Specification;
        //是否创建（激活）
        bool m_IsActive = false;
    };
}
