//
// Created by chenlong on 2026/8/23.
//
#pragma once
#include "Limen/Core/Core.h"

namespace Limen
{
    /**
    * @brief Framebuffer 的创建参数。
     *
     * 第一版固定使用：
     * - 一个 RGBA8 颜色附件；
     * - 一个 Depth24Stencil8 深度/模板附件；
     * - 使用MSAA 级别定为4
     */
    struct FramebufferSpecification
    {
        /**
         * 渲染的像素的宽度
         *
         * 不是窗口逻辑宽度；后续的Scene Viewport有多大
         * Framebuffer 就resize为多大
         */
        uint32_t Width = 1;

        /**
        * 渲染的像素的高度
        */
        uint32_t Height = 1;

        /**
         * 抗锯齿采样数。
         *
         * 1：关闭 MSAA；
         * 2、4、8：对应多重采样等级。
         *
         * 后端会检查硬件是否支持该采样数。
         */
        uint32_t Samples = 4;
    };

    /**
     * @brief 离屏渲染目标的跨后端接口 FBO：管画出来之后，结果写到哪里去（输出写到哪）
     *
     * OpenGL 对应FBO
     * D3D12 中对应 RTV/DSV 所指向的 Render Target；
     * Metal 中对应 MTLTexture + Render Pass Attachment。
     */
    class LIMEN_API Framebuffer
    {
    public:
        virtual ~Framebuffer() = default;

        /**
         * 将此 Framebuffer 设为当前渲染目标。
         *
         * 此后的 Clear、DrawIndexed 等命令都写入它的颜色和深度附件。
         */
        virtual void Bind() const = 0;

        /**
        * 解绑离屏 Framebuffer，回到窗口默认渲染目标。
        */
        virtual void UnBind() const = 0;

        /**
         * 改变离屏纹理与深度附件的尺寸。
         *
         * @param width  新的像素宽度，必须大于 0。
         * @param height 新的像素高度，必须大于 0。
         */
        virtual void Resize(uint32_t width, uint32_t height) = 0;

        /**
        * @return 此 Framebuffer 的当前规格。
        */
        [[nodiscard]]
        virtual const FramebufferSpecification &GetSpecification() const noexcept = 0;

        /**
         * @brief 取得颜色附件的后端原生句柄。
         *
         * OpenGL 当前返回 GLuint；
         * D3D12 以后可返回 SRV/描述符对应的无符号句柄；
         * Metal 可返回 MTLTexture 指针转换后的整数。
         *
         * 调用者不能对它做运算；它只用于把颜色纹理交给
         * 对应后端的 ImGui::Image 显示。
         */
        [[nodiscard]]
        virtual std::uintptr_t GetColorAttachmentHandle() const noexcept = 0;

        /**
         * @brief 将多采样颜色结果解析到可采样的普通颜色纹理。
         *
         * Samples == 1 时此函数不需要做任何工作。
         *
         * Samples > 1 时，OpenGL 使用 glBlitFramebuffer；
         * DX12 以后对应 ResolveSubresource；
         * Metal 对应 Render Pass 的 resolveTexture。
         *
         * 必须在场景绘制完成、ImGui::Image 显示前调用。
         */
        virtual void Resolve() const = 0;

        /**
         * @brief 按当前 RendererAPI 创建对应后端的 Framebuffer。
         */
        [[nodiscard]]
        static Scope<Framebuffer> Create(
            const FramebufferSpecification &specification
        );
    };
}
