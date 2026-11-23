//
// Created by chenlong on 2026/11/23.
//
#pragma once

#include <cstdint>
#include <variant>
#include <glm/vec3.hpp>

#include "Limen/Asset/AssetID.h"
#include "Limen/Core/Core.h"
#include "Limen/Scene/SceneIdentity.h"
#include "Limen/Scene/SceneTransform.h"

namespace Limen
{
    // Authoring Component 类型标签
    enum class SceneComponentKind : std::uint8_t
    {
        ModelRenderer,
        // 渲染基本的几何元素，点、线、面等
        PrimitiveRenderer,
        PointLight
    };

    // 第一版只支持立方体 Primitive
    enum class PrimitiveType : std::uint8_t
    {
        Cube
    };

    // 引用项目中的 Model 资产；运行期 Mesh/Material 由 AssetManager 解析。
    struct ModelRendererComponent
    {
        AssetID ModelAssetID{};
    };

    // 声明使用引擎内置 Primitive，不拥有运行期 Mesh/Material。
    struct PrimitiveRendererComponent
    {
        PrimitiveType Primitive = PrimitiveType::Cube;
    };

    // 点光源位置来自 SceneObjectComponent::LocalTransform。
    struct PointLightComponent
    {
        glm::vec3 Color{1.0f};
        float Intensity = 1.0f;
    };

    using SceneComponentData = std::variant<
        ModelRendererComponent,
        PrimitiveRendererComponent,
        PointLightComponent
    >;

    // SceneObject 根 Transform 下的一条可持久组件。
    // 注意：Data 默认构造为 ModelRendererComponent，
    // 但其 AssetID 无效；默认 SceneObjectComponent 不能直接添加。
    // 调用方必须显式设置 Model、Primitive 或 PointLight 数据。
    struct SceneObjectComponent
    {
        SceneComponentID ID{};
        SceneTransform LocalTransform{};
        SceneComponentData Data;
    };

    [[nodiscard]]
    LIMEN_API SceneComponentID GenerateSceneComponentID();

    [[nodiscard]]
    LIMEN_API SceneComponentKind GetSceneComponentKind(
        const SceneObjectComponent &component) noexcept;

    // 校验 ComponentID, LocalTransform 以及当前 variant 类型的必填数据
    [[nodiscard]]
    LIMEN_API bool IsSceneObjectComponentValid(
        const SceneObjectComponent& component
    ) noexcept;
}
