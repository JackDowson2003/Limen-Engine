//
// Created by chenlong on 2026/9/23.
//
#include <cassert>

#include "OpenGLGPUProfiler.h"

#include "Limen/Core/Log.h"

namespace Limen
{
    OpenGLGPUProfiler::~OpenGLGPUProfiler()
    {
        for (FrameRecord &frame: m_FrameRing)
        {
            if (frame.ElapsedQuery != 0)
                glDeleteQueries(1, &frame.ElapsedQuery);
        }

    }

    void OpenGLGPUProfiler::BeginFrame()
    {
        if (m_CurrentFrame != nullptr || m_SkippingFrame)
        {
            LM_CORE_WARN("GPUProfiler::BeginFrame called before previous EndFrame");
            return;
        }

        // 1. 环形槽：每 kFrameLatency 帧复用同一个槽
        const auto slot = static_cast<uint32_t>(m_FrameIndex % kFrameLatency);
        FrameRecord &frame = m_FrameRing[slot];

        //2. 空槽没有旧结果；占用的槽必须先完成读回。
        if (frame.HasPendingElapsedResult && !TryReadAndSaveFrameResult(frame))
        {
            m_SkippingFrame = true;
            return; // GPU 尚未完成：保留旧槽和它的 Query。
        }

        // 只有空槽或旧结果已读回，才能清空旧记录并开始本次采样
        ReleaseFrameRecord(frame);

        // 每个环形槽只创建一次，后续帧在结果读回后复用
        if (frame.ElapsedQuery == 0)
            glGenQueries(1, &frame.ElapsedQuery);

        if (frame.ElapsedQuery == 0)
        {
            LM_CORE_WARN("GPUProfiler: failed to create GL_TIME_ELAPSED query");
            m_SkippingFrame = true;
            return;
        }

        // 3. 建立当前帧并记录序号
        m_CurrentFrame = &frame;
        frame.FrameIndex = m_FrameIndex;

        // 从本帧场景命令提交前开始测量
        glBeginQuery(GL_TIME_ELAPSED, frame.ElapsedQuery);

        // 5. 新帧没有未闭合节点，节点栈清空
        m_NodeStack.clear();
    }

    bool OpenGLGPUProfiler::TryReadAndSaveFrameResult(const FrameRecord &frame)
    {
        if (!IsFrameReady(frame))
            return false;

        GLuint64 elapsedNanoseconds = 0;
        // 查询ElapsedQuery的结果，并输出给 elapsedNanoseconds 保存
        glGetQueryObjectui64v(
            frame.ElapsedQuery,
            GL_QUERY_RESULT,
            &elapsedNanoseconds
        );

        GPUProfileFrameResult completed;
        completed.FrameIndex = frame.FrameIndex;
        completed.FrameDurationMilliseconds =
                static_cast<double>(elapsedNanoseconds) / 1000000.0;
        completed.Valid = true;

        // 旧槽可能晚于新槽完成，只发布帧编号更新的结果。
        if (!m_LatestCompletedResult.Valid
            || completed.FrameIndex > m_LatestCompletedResult.FrameIndex)
            m_LatestCompletedResult = std::move(completed);

        return true;
    }

    void OpenGLGPUProfiler::ReleaseFrameRecord(FrameRecord &frame)
    {
        // 清空该槽旧帧的 CPU 记录，保留 ElapsedQuery 供后续复用。
        frame.RootNodes.clear();
        frame.FrameIndex = 0;
        frame.HasPendingElapsedResult = false;
    }

    bool OpenGLGPUProfiler::IsFrameReady(const FrameRecord &frame)
    {
        if (!frame.HasPendingElapsedResult || frame.ElapsedQuery == 0)
            return false;

        GLint available = GL_FALSE;
        // 查询结果是否已可读
        glGetQueryObjectiv(frame.ElapsedQuery, GL_QUERY_RESULT_AVAILABLE, &available);
        return available == GL_TRUE;
    }

    GPUProfileScopeHandle OpenGLGPUProfiler::BeginScope(const std::string_view debugName)
    {
        // 没有活动帧属于调用顺序错误：返回无效Handle
        if (m_CurrentFrame == nullptr)
            return GPUProfileScopeHandle{GPUProfileScopeHandle::InvalidValue};

        if (m_NextScopeHandleValue == GPUProfileScopeHandle::InvalidValue)
            return {};
        const uint64_t handleValue = m_NextScopeHandleValue++;

        // 压栈的 depth = 压栈前栈大小，也就是节点在栈中的位置
        const auto depth = static_cast<uint32_t>(m_NodeStack.size());

        // 创建独占节点：复制名字、记录深度、父节点只借用
        Scope<ProfileNode> node = CreateScope<ProfileNode>();
        node->Name = std::string(debugName);
        node->Depth = depth;
        node->HandleValue = handleValue;
        node->Parent = m_NodeStack.empty() ? nullptr : m_NodeStack.back();

        // 整帧查询进行期间这里只记录 Scope 层级与配对，不提交 Scope GPU 查询。

        // 先取裸指针（借用）；move 之后 node 变空、取不到地址了
        ProfileNode *nodePtr = node.get();

        // 挂树：栈空挂根列表，否则挂栈顶节点的 Children。
        if (m_NodeStack.empty())
            m_CurrentFrame->RootNodes.push_back(std::move(node));
        else
            // 在顶层挂载节点 看谁后来了
            m_NodeStack.back()->Children.push_back(std::move(node));

        // 压栈：记录当前未闭合路径
        m_NodeStack.push_back(nodePtr);

        // handle携带handle, EndScope 校验配对用
        return GPUProfileScopeHandle{handleValue};
    }

    void OpenGLGPUProfiler::EndScope(GPUProfileScopeHandle handle)
    {
        // 跳过采样时 BeginScope 返回无效 handle，配对结束也是正常操作。
        if (m_SkippingFrame && !handle.IsValid())
            return;

        if (m_CurrentFrame == nullptr || m_NodeStack.empty())
        {
            LM_CORE_WARN("GPUProfiler::EndScope called without an active scope");
            return;
        }

        if (ProfileNode *activeNode = m_NodeStack.back();
            !handle.IsValid() || activeNode->HandleValue != handle.Value
        )
        {
            LM_CORE_WARN(
                "GPUProfiler::EndScope handle mismatch: received {}, expected {}",
                handle.Value,
                activeNode->HandleValue
            );
            return;
        }

        m_NodeStack.pop_back();
    }

    void OpenGLGPUProfiler::EndFrame()
    {
        // 本帧只跳过 GPU 采样，扔推进 CPU 帧编号以轮换环形槽
        if (m_SkippingFrame)
        {
            m_SkippingFrame = false;
            ++m_FrameIndex;
            return;
        }
        if (m_CurrentFrame == nullptr)
        {
            LM_CORE_WARN("GPUProfiler::EndFrame called without BeginFrame");
            return;
        }
        if (!m_NodeStack.empty())
        {
            LM_CORE_WARN(
                "GPUProfiler::EndFrame has {} unclosed scopes",
                m_NodeStack.size()
            );
            return;
        }

        FrameRecord &frame = *m_CurrentFrame;

        // 测量覆盖到本帧已提交的场景与 ImGui 命令
        glEndQuery(GL_TIME_ELAPSED);
        frame.HasPendingElapsedResult = true;

        m_CurrentFrame = nullptr;
        ++m_FrameIndex;
    }

    const GPUProfileFrameResult &OpenGLGPUProfiler::GetLatestCompletedFrame() const noexcept
    {
        return m_LatestCompletedResult;
    }
}
