//
// Created by chenlong on 2026/11/23.
//

#pragma once

#include <glm/gtc/quaternion.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Limen/Core/Core.h"

namespace Limen
{
    // Scene Authoring 数据视图 TRS; Renderer 需要时转换为矩阵
    struct LIMEN_API SceneTransform
    {
        glm::vec3 Location{0.0f};
        glm::quat Rotation{1.0f,0.0f,0.0f,0.0f};
        glm::vec3 Scale{1.0f};

        [[nodiscard]]
        bool IsFinite() const noexcept;

        [[nodiscard]]
        glm::mat4 ToMatrix() const noexcept;
    };
}
