//
// Created by chenlong on 2026/11/23.
//

#include "Limen/Scene/SceneTransform.h"
#include <cmath>

#include <glm/gtc/matrix_transform.hpp>
namespace Limen
{
    bool SceneTransform::IsFinite() const noexcept
    {
        return std::isfinite(Location.x) &&
            std::isfinite(Location.y) &&
            std::isfinite(Location.z) &&
            std::isfinite(Rotation.x) &&
            std::isfinite(Rotation.y) &&
            std::isfinite(Rotation.z) &&
            std::isfinite(Rotation.w) &&
            std::isfinite(Scale.x) &&
            std::isfinite(Scale.y) &&
            std::isfinite(Scale.z);
    }

    glm::mat4 SceneTransform::ToMatrix() const noexcept
    {
        constexpr glm::mat4 identity{1.0f};
        // TRS
        return glm::translate(identity,Location) *
            glm::mat4_cast(Rotation) *
                glm::scale(identity, Scale);
    }
}
