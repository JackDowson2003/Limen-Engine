//
// Created by chenlong on 2026/10/22.
//

#pragma once

#include <cstdint>
#include <string_view>

#include "Limen/Core/Core.h"
#include "Limen/Tool/LString.h"

namespace Limen
{
    // 资产的持久身份，创建或导入的时候生成一次，移动、重命名保持不变
    struct AssetID
    {
        std::uint64_t High = 0;
        std::uint64_t Low = 0;

        [[nodiscard]]
        constexpr bool isValid() const noexcept
        {
            return High != 0 || Low != 0;
        }

        friend constexpr bool operator==(const AssetID&, const AssetID&) noexcept = default;
    };
    [[nodiscard]] LIMEN_API AssetID GenerateAssetID();
    [[nodiscard]] LIMEN_API LString AssetIDToString(AssetID id);
    [[nodiscard]] LIMEN_API bool TryParseAssetID(std::string_view text, AssetID& output) noexcept;
}
