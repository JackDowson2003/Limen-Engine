#pragma once

#include <cmath>
#include <locale>
#include <sstream>
#include <string_view>

#include <glm/trigonometric.hpp>

#include "Limen/Scene/SceneTransform.h"

namespace LimenEditor
{
    enum class DetailsTransformField
    {
        Location,
        Rotation,
        Scale
    };

    // 输入三轴值，旋转单位为度。只有三个分量都有效才写入 Transform。
    [[nodiscard]] inline bool TryApplyDetailsTransformInput(
        const DetailsTransformField field,
        const std::string_view text,
        Limen::SceneTransform &transform)
    {
        std::istringstream input{std::string(text)};
        input.imbue(std::locale::classic());
        auto separator = [&input]()
        {
            const auto next = input.peek();
            if (next != ',' && next != ' ' && next != '\t')
                return false;
            input >> std::ws;
            if (input.peek() == ',')
                input.get();
            return true;
        };

        glm::vec3 value{};
        if (!(input >> value.x) || !separator() || !(input >> value.y) ||
            !separator() || !(input >> value.z))
            return false;
        input >> std::ws;
        if (!input.eof() || !std::isfinite(value.x) || !std::isfinite(value.y) || !std::isfinite(value.z))
            return false;

        switch (field)
        {
            case DetailsTransformField::Location:
                transform.Location = value;
                return true;
            case DetailsTransformField::Rotation:
                transform.Rotation = glm::quat(glm::radians(glm::vec3{
                    std::remainder(value.x, 360.0f), std::remainder(value.y, 360.0f),
                    std::remainder(value.z, 360.0f)}));
                return true;
            case DetailsTransformField::Scale:
                transform.Scale = value;
                return true;
        }
        return false;
    }
}
