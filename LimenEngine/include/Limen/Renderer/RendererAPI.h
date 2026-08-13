//
// Created by chenlong on 2026/8/12.
//

#pragma once
#include "VertexArray.h"
#include "glm/glm.hpp"

namespace Limen
{
    class LIMEN_API RendererAPI
    {
      public:
        virtual ~RendererAPI() = default;

        enum class API
        {
            NONE = 0,
            OPENGL = 1,
            VULKAN = 2,
            DIRECT12 = 3,
            DIRECT11 = 4,
            METAL = 5,
        };
    public:

        virtual void Clear() = 0;
        virtual void SetClearColor(const glm::vec4& color) = 0;

        virtual void DrawIndexed(const VertexArray& vertexArray) = 0;

        static inline API GetAPI() { return API::OPENGL; }
    private:
        static API s_API;
    };
}
