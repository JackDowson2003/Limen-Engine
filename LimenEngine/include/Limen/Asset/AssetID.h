//
// Created by chenlong on 2026/10/22.
//

#pragma once

#include <cstdint>

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
}