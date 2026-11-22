#include <iostream>

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

    if (success)
        std::cout << "Editor viewport projection: PASS\n";
    return success ? 0 : 1;
}
