#include <cmath>
#include <iostream>

#include <glm/gtc/matrix_transform.hpp>

#include "Limen/Math/AxisAlignedBoundingBox.h"

namespace
{
    bool Check(const char *name, bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }

    bool CheckVec3(const char *name, const glm::vec3 &actual, const glm::vec3 &expected)
    {
        constexpr float tolerance = 0.00001f;
        const bool matches =
            std::abs(actual.x - expected.x) <= tolerance &&
            std::abs(actual.y - expected.y) <= tolerance &&
            std::abs(actual.z - expected.z) <= tolerance;

        if (matches)
            return true;

        std::cerr << name << ": expected ("
                  << expected.x << ", " << expected.y << ", " << expected.z
                  << "), got (" << actual.x << ", " << actual.y << ", "
                  << actual.z << ")\n";
        return false;
    }
}

int main()
{
    int failures = 0;

    Limen::AxisAlignedBoundingBox empty;
    if (!Check("default bounds are invalid", !empty.IsValid()))
        ++failures;
    if (!Check("transforming empty bounds stays invalid",
               !empty.Transformed(glm::mat4(1.0f)).IsValid()))
        ++failures;

    Limen::AxisAlignedBoundingBox bounds;
    bounds.Expand(glm::vec3(3.0f, 4.0f, 2.0f));
    if (!Check("first point makes bounds valid", bounds.IsValid()))
        ++failures;
    if (!CheckVec3("first point min", bounds.Min, glm::vec3(3.0f, 4.0f, 2.0f)))
        ++failures;
    if (!CheckVec3("first point max", bounds.Max, glm::vec3(3.0f, 4.0f, 2.0f)))
        ++failures;

    bounds.Expand(glm::vec3(-1.0f, -2.0f, 0.0f));
    if (!CheckVec3("two-point min", bounds.Min, glm::vec3(-1.0f, -2.0f, 0.0f)))
        ++failures;
    if (!CheckVec3("two-point max", bounds.Max, glm::vec3(3.0f, 4.0f, 2.0f)))
        ++failures;
    if (!CheckVec3("center", bounds.GetCenter(), glm::vec3(1.0f, 1.0f, 1.0f)))
        ++failures;
    if (!CheckVec3("extents", bounds.GetExtents(), glm::vec3(2.0f, 3.0f, 1.0f)))
        ++failures;

    bounds.Expand(empty);
    if (!CheckVec3("invalid merge leaves min unchanged", bounds.Min,
                   glm::vec3(-1.0f, -2.0f, 0.0f)))
        ++failures;
    if (!CheckVec3("invalid merge leaves max unchanged", bounds.Max,
                   glm::vec3(3.0f, 4.0f, 2.0f)))
        ++failures;

    // 列向量约定：先在局部空间旋转，再平移到目标空间。
    const glm::mat4 transform =
        glm::translate(glm::mat4(1.0f), glm::vec3(10.0f, -3.0f, 5.0f)) *
        glm::rotate(glm::mat4(1.0f), glm::radians(45.0f),
                    glm::vec3(0.0f, 0.0f, 1.0f));
    const Limen::AxisAlignedBoundingBox rotated = bounds.Transformed(transform);
    const float inverseSqrt2 = std::sqrt(0.5f);
    if (!Check("rotated bounds are valid", rotated.IsValid()))
        ++failures;
    if (!CheckVec3("rotated min", rotated.Min,
                   glm::vec3(10.0f - 5.0f * inverseSqrt2,
                             -3.0f - 3.0f * inverseSqrt2, 5.0f)))
        ++failures;
    if (!CheckVec3("rotated max", rotated.Max,
                   glm::vec3(10.0f + 5.0f * inverseSqrt2,
                             -3.0f + 7.0f * inverseSqrt2, 7.0f)))
        ++failures;

    Limen::AxisAlignedBoundingBox other;
    other.Expand(glm::vec3(-4.0f, 1.0f, -2.0f));
    other.Expand(glm::vec3(0.0f, 5.0f, 1.0f));
    bounds.Expand(other);
    if (!CheckVec3("valid merge min", bounds.Min, glm::vec3(-4.0f, -2.0f, -2.0f)))
        ++failures;
    if (!CheckVec3("valid merge max", bounds.Max, glm::vec3(3.0f, 5.0f, 2.0f)))
        ++failures;

    return failures == 0 ? 0 : 1;
}
