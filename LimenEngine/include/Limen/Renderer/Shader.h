//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include "Core.h"
#include "glm/fwd.hpp"
#include "glm/detail/type_vec4.hpp"

namespace Limen
{
    class LIMEN_API Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        // 根据当前 Renderer API 创建对应的 Shader 实现。
        [[nodiscard]] static std::unique_ptr<Shader> Create(
            const std::string &vertexSource,
            const std::string &fragmentSource
        );
    };
}
