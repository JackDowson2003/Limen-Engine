//
// Created by chenlong on 2026/11/24.
//

#pragma once

#include <cstdint>
#include <string_view>

#include "Limen/Core/Core.h"
#include "Limen/Scene/Light.h"
#include "Limen/Scene/SceneAuthoring.h"
#include "Limen/Tool/LArray.h"
#include "Limen/Tool/LString.h"

namespace Limen
{
    // 场景的持久化数据，供 .lscene 读取和写入使用
    struct SceneDocument
    {
        static constexpr std::uint32_t CurrentSchemaVersion = 1;

        std::uint32_t SchemaVersion = CurrentSchemaVersion;
        LString DisplayName;
        AmbientLight Ambient{};
        DirectionalLight DirectionLight{};
        // 对象快照
        LArray<SceneObjectAuthoringData> Objects;
    };

    [[nodiscard]]
    LIMEN_API SceneDocument CaptureSceneDocument(
        const class Scene& scene,
        std::string_view displayName
    );
}
