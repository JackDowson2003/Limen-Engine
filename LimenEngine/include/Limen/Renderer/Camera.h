#pragma once

#include "Core.h"
#include <glm/glm.hpp>

namespace Limen
{
    class LIMEN_API Camera
    {
    public:
        virtual ~Camera() = default;

        [[nodiscard]]
        virtual const glm::mat4& GetProjectionMatrix() const = 0;

        [[nodiscard]]
        virtual const glm::mat4& GetViewMatrix() const = 0;

        [[nodiscard]]
        virtual const glm::mat4& GetViewProjectionMatrix() const = 0;

        [[nodiscard]]
        virtual const glm::vec3& GetPosition() const = 0;
    };
}