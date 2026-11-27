#include <iostream>

#include <glm/gtc/matrix_transform.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>

#include "Limen/Math/AxisAlignedBoundingBox.h"
#include "UI/EditorViewportProjection.h"

namespace
{
    bool Check(const char *name, const bool condition)
    {
        if (condition)
            return true;

        std::cerr << "FAIL: " << name << '\n';
        return false;
    }

    bool TestFrameBounds()
    {
        const Limen::AxisAlignedBoundingBox bounds{
            .Min = {-1.0f, 1.0f, -4.0f},
            .Max = {3.0f, 3.0f, -2.0f}
        };
        const glm::vec3 center = bounds.GetCenter();
        const glm::vec3 originalPosition{7.0f, 6.0f, 5.0f};
        const glm::vec3 up{0.0f, 1.0f, 0.0f};
        const auto view = glm::lookAt(originalPosition, center, up);
        constexpr float nearPlane = 0.1f;
        constexpr float farPlane = 100.0f;
        const glm::mat4 wideProjection = glm::perspective(glm::radians(45.0f), 2.0f, nearPlane, farPlane);
        const glm::mat4 narrowProjection = glm::perspective(glm::radians(45.0f), 0.5f, nearPlane, farPlane);

        glm::vec3 widePosition{};
        glm::vec3 narrowPosition{};
        if (!Check("framing succeeds for wide and narrow viewports",
                   LimenEditor::TryCalculateFramingPosition(
                       view, wideProjection, bounds, nearPlane, farPlane, widePosition) &&
                   LimenEditor::TryCalculateFramingPosition(
                       view, narrowProjection, bounds, nearPlane, farPlane, narrowPosition)))
            return false;

        const glm::vec3 backward = glm::normalize(originalPosition - center);
        bool success = Check("framing preserves the camera orientation",
                             glm::dot(glm::normalize(narrowPosition - center), backward) > 0.9999f);
        success = Check("narrow viewport requires more framing distance",
                        glm::length(narrowPosition - center) > glm::length(widePosition - center)) &&
                  success;

        constexpr LimenEditor::UIRect viewport{0.0f, 0.0f, 100.0f, 200.0f};
        const glm::mat4 framedView = glm::lookAt(narrowPosition, center, up);
        glm::vec2 min{};
        glm::vec2 max{};
        success = Check("framed bounds have visible margin on all four sides",
                        LimenEditor::TryProjectWorldBoundsToScreen(
                            narrowProjection * framedView, viewport, bounds, min, max) &&
                        min.x > 5.0f && min.y > 5.0f &&
                        max.x < viewport.Width - 5.0f && max.y < viewport.Height - 5.0f) &&
                  success;

        const Limen::AxisAlignedBoundingBox pointBounds{.Min = center, .Max = center};
        glm::vec3 pointPosition{};
        success = Check("point selection has room between it and the camera",
                        LimenEditor::TryCalculateFramingPosition(
                            view, wideProjection, pointBounds, nearPlane, farPlane, pointPosition) &&
                        glm::length(pointPosition - center) > nearPlane) &&
                  success;

        const Limen::AxisAlignedBoundingBox unframeableBounds{
            .Min = glm::vec3{-500.0f}, .Max = glm::vec3{500.0f}
        };
        const glm::vec3 sentinel{21.0f, 22.0f, 23.0f};
        auto failedPosition = sentinel;
        success = Check("bounds outside the camera range leave its position unchanged",
                        !LimenEditor::TryCalculateFramingPosition(
                            view, wideProjection, unframeableBounds, nearPlane, farPlane, failedPosition) &&
                        failedPosition == sentinel) &&
                  success;
        return success;
    }
}

int main()
{
    const LimenEditor::UIRect viewport{
        100.0f,
        50.0f,
        200.0f,
        100.0f
    };
    const Limen::AxisAlignedBoundingBox unitCube{
        .Min = {-0.5f, -0.5f, -0.5f},
        .Max = {0.5f, 0.5f, 0.5f}
    };

    glm::vec2 min{};
    glm::vec2 max{};
    bool success = true;

    success = Check(
                  "identity projection succeeds",
                  LimenEditor::TryProjectWorldBoundsToScreen(
                      glm::mat4{1.0f},
                      viewport,
                      unitCube,
                      min,
                      max)) &&
              success;
    success = Check(
                  "projected minimum matches viewport mapping",
                  min.x == 150.0f && min.y == 75.0f) &&
              success;
    success = Check(
                  "projected maximum matches viewport mapping",
                  max.x == 250.0f && max.y == 125.0f) &&
              success;

    Limen::AxisAlignedBoundingBox invalidBounds;
    min = {123.0f, 123.0f};
    max = {456.0f, 456.0f};
    success = Check(
                  "invalid bounds fail",
                  !LimenEditor::TryProjectWorldBoundsToScreen(
                      glm::mat4{1.0f},
                      viewport,
                      invalidBounds,
                      min,
                      max)) &&
              success;
    success = Check(
                  "failed projection preserves outputs",
                  min.x == 123.0f && min.y == 123.0f &&
                  max.x == 456.0f && max.y == 456.0f) &&
              success;

    success = TestFrameBounds() && success;

    if (success)
        std::cout << "Editor viewport projection: PASS\n";
    return success ? 0 : 1;
}
