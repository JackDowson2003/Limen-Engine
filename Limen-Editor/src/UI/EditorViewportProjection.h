#pragma once

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <limits>

#include <glm/geometric.hpp>
#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

#include "Limen/Math/AxisAlignedBoundingBox.h"
#include "UI/EditorUILayout.h"

namespace LimenEditor
{
    [[nodiscard]] inline bool IsFiniteViewportPoint(const glm::vec3 &point) noexcept
    {
        return std::isfinite(point.x) && std::isfinite(point.y) && std::isfinite(point.z);
    }

    // 保持相机朝向，让包围盒的每个角点落在透视视锥内。
    [[nodiscard]] inline bool TryCalculateFramingPosition(
        const glm::mat4 &view,
        const glm::mat4 &projection,
        const Limen::AxisAlignedBoundingBox &bounds,
        const float nearPlane,
        const float farPlane,
        glm::vec3 &outPosition) noexcept
    {
        if (!bounds.IsValid() || !IsFiniteViewportPoint(bounds.Min) || !IsFiniteViewportPoint(bounds.Max) ||
            !std::isfinite(nearPlane) || !std::isfinite(farPlane) || nearPlane <= 0.0f || farPlane <= nearPlane)
            return false;

        // 透视投影的两个缩放分量分别等于水平、垂直半视场角正切的倒数。
        const float horizontalScale = projection[0][0];
        const float verticalScale = projection[1][1];
        if (!std::isfinite(horizontalScale) || !std::isfinite(verticalScale) ||
            horizontalScale <= 0.0f || verticalScale <= 0.0f)
            return false;

        const glm::vec3 center = bounds.Min * 0.5f + bounds.Max * 0.5f;
        const glm::vec3 extents = bounds.Max * 0.5f - bounds.Min * 0.5f;
        const glm::mat3 viewRotation{view};
        constexpr float occupiedNdcExtent = 0.82f;
        constexpr float minimumDistance = 1.5f;
        float distance = minimumDistance;
        float minimumCameraZ = 0.0f;
        for (const float x : {-extents.x, extents.x})
        {
            for (const float y : {-extents.y, extents.y})
            {
                for (const float z : {-extents.z, extents.z})
                {
                    const glm::vec3 corner = viewRotation * glm::vec3{x, y, z};
                    if (!IsFiniteViewportPoint(corner))
                        return false;

                    // 相机后退 distance 后，角点的正向深度为 distance - corner.z。
                    const float minimumDepth = std::max({
                        std::abs(corner.x) * horizontalScale / occupiedNdcExtent,
                        std::abs(corner.y) * verticalScale / occupiedNdcExtent,
                        nearPlane * 1.1f
                    });
                    distance = std::max(distance, corner.z + minimumDepth);
                    minimumCameraZ = std::min(minimumCameraZ, corner.z);
                }
            }
        }
        if (distance - minimumCameraZ >= farPlane)
            return false;

        const glm::vec3 backward{view[0][2], view[1][2], view[2][2]};
        const glm::vec3 candidate = center + backward * distance;
        if (!IsFiniteViewportPoint(candidate) || glm::dot(candidate - center, backward) <= 0.0f)
            return false;

        outPosition = candidate;
        return true;
    }

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
