//
// Created by chenlong on 2026/11/23.
//

#pragma once

#include <cstdint>
#include <string_view>

#include "Limen/Core/Core.h"
#include "Limen/Tool/LString.h"

namespace Limen
{
    // 写入场景文件的对象身份，与AssetID 和运行期 Handle 不同
    struct SceneObjectID
    {
        std::uint64_t High = 0;
        std::uint64_t Low = 0;

        [[nodiscard]]
        constexpr bool isValid() const noexcept
        {
            return High != 0 || Low != 0;
        }

        friend constexpr bool operator==(const SceneObjectID &, const SceneObjectID &) noexcept = default;
    };

    [[nodiscard]]
    LIMEN_API LString SceneObjectIDToString(SceneObjectID id);

    [[nodiscard]]
    LIMEN_API bool TryParseSceneObjectID(
        std::string_view text,
        SceneObjectID &output
    ) noexcept;

    // SceneObject 内部组件的持久身份。
    struct SceneComponentID
    {
        std::uint64_t High = 0;
        std::uint64_t Low = 0;

        [[nodiscard]]
        constexpr bool isValid() const noexcept
        {
            return High != 0 || Low != 0;
        }

        friend constexpr bool operator==(
            const SceneComponentID &,
            const SceneComponentID &) noexcept = default;
    };

    [[nodiscard]]
    LIMEN_API LString SceneComponentIDToString(SceneComponentID id);

    [[nodiscard]]
    LIMEN_API bool TryParseSceneComponentID(
        std::string_view text,
        SceneComponentID &output
    ) noexcept;
}
