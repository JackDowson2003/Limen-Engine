//
// Created by chenlong on 2026/11/23.
//

#include <cstddef>
#include <random>
#include <type_traits>
#include <cmath>
#include "Limen/Scene/SceneComponent.h"

namespace Limen
{
    SceneComponentID GenerateSceneComponentID()
    {
        std::uint8_t bytes[16]{};
        std::random_device device;
        std::uniform_int_distribution<unsigned int> byte(0, 255);

        for (auto &value: bytes)
            value = static_cast<std::uint8_t>(byte(device));

        // UUID v4 版本位
        bytes[6] = static_cast<std::uint8_t>((bytes[6] & 0x0f) | 0x40);

        // RFC 4122 variant 位。
        bytes[8] = static_cast<std::uint8_t>((bytes[8] & 0x3f) | 0x80);

        SceneComponentID id{};
        for (std::size_t i = 0; i < 8; ++i)
            id.High = (id.High << 8) | bytes[i];
        for (std::size_t i = 8; i < 16; ++i)
            id.Low = (id.Low << 8) | bytes[i];

        return id;
    }

    SceneComponentKind GetSceneComponentKind(const SceneObjectComponent &component) noexcept
    {
        return std::visit(
            [](const auto &data) noexcept
            {
                using Data = std::decay_t<decltype(data)>;
                if constexpr (std::is_same_v<Data, ModelRendererComponent>)
                {
                    return SceneComponentKind::ModelRenderer;
                } else if constexpr (std::is_same_v<Data, PrimitiveRendererComponent>)
                {
                    return SceneComponentKind::PrimitiveRenderer;
                } else
                {
                    static_assert(std::is_same_v<
                        Data,
                        PointLightComponent>);
                    return SceneComponentKind::PointLight;
                }
            },
            component.Data
        );
    }

    bool IsSceneObjectComponentValid(const SceneObjectComponent &component) noexcept
    {
        if (!component.ID.isValid() || !component.LocalTransform.IsFinite())
            return false;

        return std::visit(
            []<typename T>(const T &data) noexcept
            {
                using Data = std::decay_t<T>;

                if constexpr (std::is_same_v<Data, ModelRendererComponent>)
                {
                    return data.ModelAssetID.isValid();
                } else if constexpr (std::is_same_v<Data, PrimitiveRendererComponent>)
                {
                    return data.Primitive == PrimitiveType::Cube;
                } else
                {
                    static_assert(std::is_same_v<Data, PointLightComponent>);

                    return std::isfinite(data.Color.x) &&
                           std::isfinite(data.Color.y) &&
                           std::isfinite(data.Color.z) &&
                           std::isfinite(data.Intensity) &&
                           data.Intensity >= 0.0f;
                }
            },
            component.Data
        );
    }
}