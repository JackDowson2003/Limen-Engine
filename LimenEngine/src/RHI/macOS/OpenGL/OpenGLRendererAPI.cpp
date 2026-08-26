//
// Created by chenlong on 2026/8/12.
//

#include <glad/gl.h>
#include "OpenGLRendererAPI.h"

#include "Limen/Core/Log.h"

namespace Limen
{
    namespace
    {
        /**
         * @brief 将通用图元类型转换为OpenGL绘制模式。
         */
        GLenum ToOpenGLPrimitiveTopology(const PrimitiveTopology topology)
        {
            switch (topology)
            {
                case PrimitiveTopology::TriangleList:
                    return GL_TRIANGLES;

                case PrimitiveTopology::LineList:
                    return GL_LINES;

                case PrimitiveTopology::PointList:
                    return GL_POINTS;
            }

            LM_CORE_ERROR(
                "Unknown primitive topology"
            );

            return GL_TRIANGLES;
        }
    }

    OpenGLRendererAPI::~OpenGLRendererAPI()
    {
    }

    void OpenGLRendererAPI::Init()
    {
        glEnable(GL_BLEND);
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }

    inline void OpenGLRendererAPI::Clear()
    {
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    inline void OpenGLRendererAPI::SetClearColor(const glm::vec4 &color)
    {
        glClearColor(color.r, color.g, color.b, color.a);
    }

    void OpenGLRendererAPI::SetDepthTest(const bool enabled)
    {
        if (enabled)
            glEnable(GL_DEPTH_TEST);
        else
            glDisable(GL_DEPTH_TEST);
    }

    void OpenGLRendererAPI::DrawIndexed(const VertexArray &vertexArray, PrimitiveTopology topology, uint32_t indexCount)
    {
        // 获取 VAO 当前绑定的 IndexBuffer。
        const Ref<IndexBuffer> &indexBuffer =
                vertexArray.GetIndexBuffer();
        LM_CORE_ASSERT(
            indexBuffer,
            "OpenGLRendererAPI::DrawIndexed requires an IndexBuffer"
        );

        if (!indexBuffer)
            return;

        // indexCount 为0时，保持旧行为：
        // 绘制 IndexBuffer 中的全部索引。
        const uint32_t actualIndexCount =
            indexCount == 0
                ? indexBuffer->GetCount()
                : indexCount;

        // 防止绘制数量超过 IndexBuffer 实际容量。
        LM_CORE_ASSERT(
            actualIndexCount <= indexBuffer->GetCount(),
            "DrawIndexed index count exceeds IndexBuffer capacity"
        );

        if (actualIndexCount > indexBuffer->GetCount())
            return;

        // 没有索引时不向 GPU 提交空绘制命令。
        if (actualIndexCount == 0)
            return;

        //按照我给的这张‘索引地图’，从你现有的顶点数据里，把指定的顶点取出来，画成我想要的图形
        glDrawElements(
                ToOpenGLPrimitiveTopology(topology),
                static_cast<GLsizei>(actualIndexCount),
                GL_UNSIGNED_INT,
                nullptr
            );
    }


    void OpenGLRendererAPI::SetViewport(const uint32_t x, const uint32_t y, const uint32_t width, const uint32_t height)
    {
        glViewport(static_cast<int>(x), static_cast<int>(y), static_cast<int>(width), static_cast<int>(height));
    }
}
