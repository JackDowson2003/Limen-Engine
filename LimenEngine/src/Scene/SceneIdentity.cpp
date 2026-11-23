//
// Created by chenlong on 2026/11/23.
//

#include <cstdint>
#include "Limen/Tool/LString.h"
#include "Limen/Scene/SceneIdentity.h"


namespace Limen
{
    namespace
    {
        LString FormatUUID128(
            const std::uint64_t high,
            const std::uint64_t low)
        {
            constexpr char hex[] = "0123456789abcdef";

            LString result;
            result.reserve(36);

            for (std::size_t i = 0; i < 16; ++i)
            {
                if (i == 4 || i == 6 || i == 8 || i == 10)
                    result.push_back('-');

                const std::uint64_t half = i < 8 ? high : low;
                const unsigned shift =
                        static_cast<unsigned>((7 - i % 8) * 8);
                const unsigned value =
                        static_cast<unsigned>((half >> shift) & 0xff);

                result.push_back(hex[value >> 4]);
                result.push_back(hex[value & 0x0f]);
            }

            return result;
        }

        bool TryParseUUID128(
            const std::string_view text,
            std::uint64_t &outHigh,
            std::uint64_t &outLow) noexcept
        {
            if (text.size() != 36)
                return false;

            const auto hexValue = [](const char character) noexcept -> int
            {
                if (character >= '0' && character <= '9')
                    return character - '0';
                if (character >= 'a' && character <= 'f')
                    return character - 'a' + 10;
                return -1;
            };

            std::uint64_t high = 0;
            std::uint64_t low = 0;
            std::size_t digitIndex = 0;

            for (std::size_t i = 0; i < text.size(); ++i)
            {
                if (i == 8 || i == 13 || i == 18 || i == 23)
                {
                    if (text[i] != '-')
                        return false;
                    continue;
                }

                const int digit = hexValue(text[i]);
                if (digit < 0)
                    return false;

                std::uint64_t &half = digitIndex < 16 ? high : low;
                half = (half << 4) | static_cast<std::uint64_t>(digit);
                ++digitIndex;
            }

            if (digitIndex != 32)
                return false;

            outHigh = high;
            outLow = low;
            return true;
        }
    }


    // 转换为:
    // xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx
    // 8      4    4    4      12
    LString SceneObjectIDToString(SceneObjectID id)
    {
        return FormatUUID128(id.High, id.Low);
    }

    bool TryParseSceneObjectID(std::string_view text, SceneObjectID &output) noexcept
    {
        std::uint64_t high = 0;
        std::uint64_t low = 0;
        if (!TryParseUUID128(text, high, low))
            return false;

        const SceneObjectID parsed{.High = high, .Low = low};
        if (!parsed.isValid())
            return false;

        output = parsed;
        return true;
    }


    LString SceneComponentIDToString(const SceneComponentID id)
    {
        return FormatUUID128(id.High, id.Low);
    }

    bool TryParseSceneComponentID(
        const std::string_view text,
        SceneComponentID &output) noexcept
    {
        std::uint64_t high = 0;
        std::uint64_t low = 0;
        if (!TryParseUUID128(text, high, low))
            return false;

        const SceneComponentID parsed{.High = high, .Low = low};
        if (!parsed.isValid())
            return false;

        output = parsed;
        return true;
    }
}
