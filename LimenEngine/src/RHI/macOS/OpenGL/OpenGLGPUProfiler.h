//
// Created by chenlong on 2026/9/23.
//

#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

#include <glad/gl.h>

#include "Limen/Renderer/GPUProfiler.h"


namespace Limen
{
    /**
     * MacOS OpenGL 4.1 的计时后端
     *
     * Renderer 通过 GPUProfiler 接口独占这个对象
     * 上层代码不能直接依赖着具体类型
     */
    class OpenGLGPUProfiler final : public GPUProfiler
    {
    public:
        OpenGLGPUProfiler() = default;

        /**
         * 在 OpenGL Context 仍然有效时释放全部查询对象
         */
        ~OpenGLGPUProfiler() override;

        // Deleted copied constructor and operator sign
        OpenGLGPUProfiler(const OpenGLGPUProfiler &) = delete;
        OpenGLGPUProfiler &operator=(const OpenGLGPUProfiler &) = delete;

        void BeginFrame() override;

        [[nodiscard]]
        GPUProfileScopeHandle BeginScope(std::string_view debugName) override;

        void EndScope(GPUProfileScopeHandle handle) override;

        void EndFrame() override;

        [[nodiscard]]
        const GPUProfileFrameResult &GetLatestCompletedFrame() const noexcept override;

    private:
        // 当前 OpenGL 后端只测整帧；节点保存具名 Scope 的 CPU 层级与配对信息。
        // BeginScope 创建节点，EndScope 校验 handle；节点不拥有 GL Query。
        struct ProfileNode
        {
            // 复制名字：不能存入参 string_view（不拥有内存，可能悬空）。
            std::string Name;

            // 创建时的嵌套深度（当时 m_NodeStack 的大小）；当前仅保存在 CPU 树中。
            uint32_t Depth = 0;

            // 用于核对 EndScope()传入的Handle, 不是GPU资源
            uint64_t HandleValue = GPUProfileScopeHandle::InvalidValue;

            // 指向父节点的非拥有指针；当前记录树关系，EndScope 通过 m_NodeStack 退栈。
            ProfileNode *Parent = nullptr;

            // 子节点独占拥有，整棵树所有权由此串起。
            std::vector<Scope<ProfileNode> > Children;
        };

        // 一整帧GPU 工作的暂存记录，唤醒缓冲按槽复用（对齐 UE FGPUProfierEventNodeFrame）
        struct FrameRecord
        {
            // 顶层根节点 （Shadow Pass / Main Pass / UI 等并列阶段），独占拥有整棵树
            std::vector<Scope<ProfileNode> > RootNodes;

            // 帧序号：读回时复制到结果，也用于核对环形槽对应哪一帧
            uint64_t FrameIndex = 0;

            // 本槽复用的整帧 GL_TIME_ELAPSED 查询；0 表示尚未创建。
            GLuint ElapsedQuery = 0;

            // glEndQuery 已提交、结果尚未由本槽读回；不表示 GPU 已完成。
            bool HasPendingElapsedResult = false;
        };

        // 复用槽位前清空旧节点和暂存状态；保留本槽 ElapsedQuery
        static void ReleaseFrameRecord(FrameRecord &frame);

        [[nodiscard]]
        static bool IsFrameReady(const FrameRecord &frame);

        // true 表示旧结果已读回、本槽可复用；ElapsedQuery 仍由本槽持有。
        [[nodiscard]]
        bool TryReadAndSaveFrameResult(const FrameRecord &frame);

        // --------- 帧 / 树 运转状态 ----
        static constexpr uint32_t kFrameLatency = 3;

        // 环形帧槽：延迟 3 帧读回，slot = FrameIndex % 3。
        FrameRecord m_FrameRing[kFrameLatency];

        // 单调递增的帧序号。
        uint64_t m_FrameIndex = 0;

        // 每创建一个 Scope 分配一个新标识，跨帧持续递增。
        uint64_t m_NextScopeHandleValue = 0;

        // 当前正在录制的帧槽（借用指针，不拥有）。
        FrameRecord *m_CurrentFrame = nullptr;

        // 旧槽不可读取的时候，仅跳过本次的 Profiler 采样，不影响场景渲染
        bool m_SkippingFrame = false;

        // 当前未闭合的节点路径，决定新节点挂载位置与 Depth（借用指针）。
        std::vector<ProfileNode *> m_NodeStack;

        // 最近一次读回完成的成品帧。
        GPUProfileFrameResult m_LatestCompletedResult;
    };
}
