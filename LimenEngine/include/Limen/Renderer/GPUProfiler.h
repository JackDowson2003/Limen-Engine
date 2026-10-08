//
// Created by chenlong on 2026/9/21.
//

#pragma once

#include <limits>
#include <cstdint>
#include <string_view>
#include <string>
#include <vector>
#include "Limen/Core/Core.h"

namespace Limen
{
    /**
     * 当前 Profiler 分配给一个计时范围的标识。
     *
     * Value 不是树的深度，也不是 OpenGL Query ID。
     */
    struct GPUProfileScopeHandle
    {
        static constexpr uint64_t InvalidValue =
            std::numeric_limits<uint64_t>::max();

        uint64_t Value = InvalidValue;

        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return Value != InvalidValue;
        }
    };

    /**
     * 一个已经完成的具名 GPU 计时结果。
     */
    struct GPUProfileScopeResult
    {
        std::string Name;

        // 嵌套深度：0 表示帧的根级范围(Shadow/ Main)
        // 每向内嵌套一层+1，调试界面根据此做树形缩进
        uint32_t Depth = 0;

        // 后端取到纳秒结果后，统一转换成便于界面显示的毫秒。
        double DurationMilliseconds = 0.0;
    };

    /**
     * 最近一次已完成的 GPU 帧计时结果；具名范围结果可选。
     */
    struct GPUProfileFrameResult
    {
        /*
         * false 表示尚无可安全读取的整帧 GPU 查询结果。
         * 耗时可能为 0，不能用 FrameDurationMilliseconds == 0 判断有效性。
         */
        bool Valid = false;

        // Profiler 内部递增的 CPU 提交帧编号。
        uint64_t FrameIndex = 0;

        // 整帧 GPU 耗时，单位毫秒；计时方式由后端决定，仅 Valid 为 true 时使用。
        double FrameDurationMilliseconds = 0.0;

        // 已测得的具名范围结果；后端不支持范围计时时可以为空。
        std::vector<GPUProfileScopeResult> Scopes;
    };


    /**
     * 后端无关的 GPU 性能测量接口
     *
     * Renderer 独占其实现
     * RenderPass 只保存BeginScope() 返回的临时Handle
     * 调试界面只读借用已经完成的结果
     */
    class LIMEN_API GPUProfiler
    {
    public:
        virtual ~GPUProfiler() = default;

        /**
         * 开启一个新的 GPU 帧提交
         */
        virtual void BeginFrame() = 0;

        /**
         * 开始具名范围，返回供 EndScope 配对的 handle。
         * 后端可以测量此范围的 GPU 耗时，也可以只记录层级和配对。
         * 若实现需要在调用返回后使用 debugName，必须复制其内容。
         */
        [[nodiscard]]
        virtual GPUProfileScopeHandle
        BeginScope(std::string_view debugName) = 0;

        /**
         * 结束与 handle 对应的具名范围。
         * 采样被跳过时，配对传入的无效 handle 是正常情况。
         */
        virtual void EndScope(
            GPUProfileScopeHandle handle
        ) = 0;

        /**
         * 结束当前帧的 GPU 测量。
         * 查询结果可以在后续帧异步发布，不等待当前帧的 GPU 工作完成。
         */
        virtual void EndFrame() = 0;

        /**
        * 返回最近一帧已经完成的结果。
        *
        * Profiler 拥有返回对象；调用者只能在 Profiler
        * 存活期间只读借用。
        */
        [[nodiscard]]
        virtual const GPUProfileFrameResult&
        GetLatestCompletedFrame() const noexcept = 0;

        /**
         * 根据当前 RendererAPI 创建对应的后端实现。
         */
        [[nodiscard]]
        static Scope<GPUProfiler> Create();
    };
}
