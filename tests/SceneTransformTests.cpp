#include "Limen/Scene/SceneTransform.h"

#include <cmath>
#include <iostream>
#include <limits>
#include <string_view>

#include <glm/gtc/quaternion.hpp>
#include <glm/trigonometric.hpp>
#include <glm/vec4.hpp>

namespace
{
    struct TestRunner
    {
        int Failures = 0;

        void Check(const std::string_view name, const bool passed)
        {
            if (passed)
                return;

            ++Failures;
            std::cerr << "FAIL: " << name << '\n';
        }
    };

    bool NearlyEqual(const float actual, const float expected)
    {
        return std::abs(actual - expected) <= 1.0e-5f;
    }

    bool NearlyEqual(const glm::vec4 &actual, const glm::vec4 &expected)
    {
        return NearlyEqual(actual.x, expected.x) &&
               NearlyEqual(actual.y, expected.y) &&
               NearlyEqual(actual.z, expected.z) &&
               NearlyEqual(actual.w, expected.w);
    }

    void TestDefaultTransform(TestRunner &runner)
    {
        const Limen::SceneTransform transform;
        runner.Check("default TRS is finite", transform.IsFinite());

        const glm::mat4 matrix = transform.ToMatrix();
        runner.Check(
            "default TRS is identity",
            matrix == glm::mat4{1.0f});
    }

    void TestTranslationAndScale(TestRunner &runner)
    {
        const Limen::SceneTransform translated{
            .Location = {3.0f, -2.0f, 5.0f}
        };
        runner.Check(
            "translation moves a point",
            NearlyEqual(
                translated.ToMatrix() * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
                {4.0f, -2.0f, 5.0f, 1.0f}));

        const Limen::SceneTransform scaled{
            .Scale = {2.0f, 3.0f, 4.0f}
        };
        runner.Check(
            "scale multiplies each axis",
            NearlyEqual(
                scaled.ToMatrix() * glm::vec4{1.0f, 1.0f, 1.0f, 1.0f},
                {2.0f, 3.0f, 4.0f, 1.0f}));
    }

    void TestRotationAndOrder(TestRunner &runner)
    {
        Limen::SceneTransform rotated;
        rotated.Rotation = glm::angleAxis(
            glm::radians(90.0f),
            glm::vec3{0.0f, 0.0f, 1.0f});
        runner.Check(
            "positive Z rotation maps X to Y",
            NearlyEqual(
                rotated.ToMatrix() * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
                {0.0f, 1.0f, 0.0f, 1.0f}));

        Limen::SceneTransform combined;
        combined.Location = {10.0f, 20.0f, 30.0f};
        combined.Rotation = glm::angleAxis(
            glm::radians(90.0f),
            glm::vec3{0.0f, 0.0f, 1.0f});
        combined.Scale = {2.0f, 3.0f, 4.0f};

        runner.Check(
            "TRS applies translate after rotate after scale",
            NearlyEqual(
                combined.ToMatrix() * glm::vec4{1.0f, 0.0f, 0.0f, 1.0f},
                {10.0f, 22.0f, 30.0f, 1.0f}));
    }

    void TestFiniteValidation(TestRunner &runner)
    {
        const float nan = std::numeric_limits<float>::quiet_NaN();
        const float infinity = std::numeric_limits<float>::infinity();

        Limen::SceneTransform invalidLocation;
        invalidLocation.Location.x = nan;
        runner.Check(
            "NaN location is rejected",
            !invalidLocation.IsFinite());

        Limen::SceneTransform invalidRotation;
        invalidRotation.Rotation.y = infinity;
        runner.Check(
            "infinite rotation is rejected",
            !invalidRotation.IsFinite());

        Limen::SceneTransform invalidScale;
        invalidScale.Scale.z = nan;
        runner.Check(
            "NaN scale is rejected",
            !invalidScale.IsFinite());
    }
}

int main()
{
    TestRunner runner;
    TestDefaultTransform(runner);
    TestTranslationAndScale(runner);
    TestRotationAndOrder(runner);
    TestFiniteValidation(runner);
    return runner.Failures == 0 ? 0 : 1;
}
