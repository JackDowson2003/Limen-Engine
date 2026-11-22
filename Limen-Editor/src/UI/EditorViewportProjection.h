#pragma once

#include <algorithm>
#include <limits>

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Limen/Math/AxisAlignedBoundingBox.h"
#include "UI/EditorUILayout.h"

namespace LimenEditor
{
    // 将世界空间 AABB 的 8 个角点投影为视口内屏幕矩形。
    [[nodiscard]] inline bool TryProjectWorldBoundsToScreen(
        const glm::mat4 &viewProjection,
        const UIRect &viewport,
        const Limen::AxisAlignedBoundingBox &bounds,
        glm::vec2 &outMin,
        glm::vec2 &outMax) noexcept
    {
        if (!bounds.IsValid() ||
            viewport.Width <= 0.0f ||
            viewport.Height <= 0.0f)
            return false;

        glm::vec2 projectedMin{
            std::numeric_limits<float>::max(),
            std::numeric_limits<float>::max()};
        glm::vec2 projectedMax{
            std::numeric_limits<float>::lowest(),
            std::numeric_limits<float>::lowest()};

        for (int x = 0; x < 2; ++x)
        {
            for (int y = 0; y < 2; ++y)
            {
                for (int z = 0; z < 2; ++z)
                {
                    const glm::vec3 corner{
                        x == 0 ? bounds.Min.x : bounds.Max.x,
                        y == 0 ? bounds.Min.y : bounds.Max.y,
                        z == 0 ? bounds.Min.z : bounds.Max.z
                    };
                    const glm::vec4 clip =
                        viewProjection * glm::vec4{corner, 1.0f};
                    if (clip.w <= 1.0e-6f)
                        return false;

                    const glm::vec3 ndc{clip / clip.w};
                    const glm::vec2 screen{
                        viewport.X + (ndc.x * 0.5f + 0.5f) * viewport.Width,
                        viewport.Y + (0.5f - ndc.y * 0.5f) * viewport.Height
                    };
                    projectedMin.x = std::min(projectedMin.x, screen.x);
                    projectedMin.y = std::min(projectedMin.y, screen.y);
                    projectedMax.x = std::max(projectedMax.x, screen.x);
                    projectedMax.y = std::max(projectedMax.y, screen.y);
                }
            }
        }

        const glm::vec2 visibleMin{
            std::max(projectedMin.x, viewport.X),
            std::max(projectedMin.y, viewport.Y)};
        const glm::vec2 visibleMax{
            std::min(projectedMax.x, viewport.X + viewport.Width),
            std::min(projectedMax.y, viewport.Y + viewport.Height)};
        if (visibleMin.x >= visibleMax.x ||
            visibleMin.y >= visibleMax.y)
            return false;

        outMin = visibleMin;
        outMax = visibleMax;
        return true;
    }
}
