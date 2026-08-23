//
// Created by chenlong on 2026/8/23.
//

#pragma once

#include <string>

#include "glm/vec4.hpp"
#include "Limen/Core/Core.h"

namespace Limen
{
    class Framebuffer;

    /**
     * @brief 描述一个渲染阶段的目标、清屏值与调试名称。
     *
     * 当前版本在 Begin() 时固定清除颜色和深度，在 End() 时执行 MSAA
     * Resolve。未来会增加 Load/Clear/DontCare 与 Store/DontCare 操作。
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
        bool m_IsActive = false;
    };
}
