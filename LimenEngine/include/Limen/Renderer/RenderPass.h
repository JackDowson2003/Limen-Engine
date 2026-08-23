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
     * @brief 描述一个渲染阶段需要使用的目标和清屏的参数
     *
     * 第一版固定在Begin的时候清除颜色和深度
     * End的时候 完成 MSAA Resolve
     *
     * 后续会扩展Load、Clear、DontCare、Store等操作
     *
     * Load Action：Pass 开始时，附件里的旧数据怎么处理？
       Load：读取旧内容（保留）
       Clear：清空为指定值
       DontCare：不关心旧内容，可以不清除、不加载
       Store Action：Pass 结束时，附件里的新数据怎么处理？
       Store：写回主存，供后续使用
       DontCare：不关心结果，可以不写回，内容变为未定义
     */
    struct RenderPassSpecification
    {

        /**
         * 当前的渲染目标
         *
         * 非拥有指针:
         * RenderPass 不负责销毁 Framebuffer
         * Framebuffer 必须要比RenderPass活得久
         * 不能让 RenderPass 再用一个 Scope<Framebuffer> 保存同一个对象，否则会出现重复所有权。
         * 也暂时不使用 Ref，因为当前 Framebuffer 明确由 Scene Viewport 独占。
         */
        Framebuffer *TargetFramebuffer = nullptr;

        /**
         *
         * Begin时用于清理颜色附件的颜色
         */
        glm::vec4 ClearColor{0.1f, 0.1f, 0.1f, 1.f};

        /**
         *
         * 用于日志、调试器和未来的GPU Debug Marker
         *
         */
        std::string DebugName = "Unnamed RenderPass";
    };

    /**
     * @brief 管理一个完整渲染过程阶段的开始和结束
     *
     * Begin:
     * - 绑定 targetFramebuffer
     * - 设置 clearColor
     * - 清楚颜色和深度
     *
     * End:
     * - 解绑 targetFramebuffer
     * - 如果开启MSAA,则执行 resolve
     */
    class LIMEN_API RenderPass final
    {
    public:
        explicit RenderPass(const RenderPassSpecification &spec);

        ~RenderPass() = default;

        RenderPass(const RenderPass &) = delete; //不允许复制
        RenderPass &operator=(const RenderPass &) = delete;

        /**
         * 开始当前的渲染阶段
         */
        void Begin();

        /**
         * 结束当前的渲染阶段
         */
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
