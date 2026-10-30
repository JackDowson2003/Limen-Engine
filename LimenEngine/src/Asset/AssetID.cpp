//
// Created by chenlong on 2026/10/23.
//

#include <random>
#include "Limen/Asset/AssetID.h"

#include "Limen/Core/Log.h"


namespace Limen
{
    AssetID GenerateAssetID()
    {
        unsigned char bytes[16]{};
        std::random_device device;
        std::uniform_int_distribution<unsigned int> byte(0, 255);
        for (auto& value : bytes)
            value = static_cast<unsigned char>(byte(device));

        bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40); // v4
        bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80); // variant

        AssetID id{};
        for (std::size_t i = 0; i < 8; ++i)
            id.High = (id.High << 8) | bytes[i];
        for (std::size_t i = 8; i < 16; ++i)
            id.Low = (id.Low << 8) | bytes[i];
        return id;
    }

    LString AssetIDToString(const AssetID id)
    {
        constexpr char hex[] = "0123456789abcdef";
        LString result;
        result.reserve(36);

        for (std::size_t i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                result.push_back('-');

            const std::uint64_t half = i < 8 ? id.High : id.Low;
            const unsigned shift = static_cast<unsigned>((7 - i % 8) * 8);
            const unsigned value = static_cast<unsigned>((half >> shift) & 0xff);
            result.push_back(hex[value >> 4]);
            result.push_back(hex[value & 0x0f]);
        }
        return result;
    }

    // ID: 12345678-1234-1234-1234-12345678.....16
    bool TryParseAssetID(const std::string_view text, AssetID& output) noexcept
    {
        if (text.size() != 36)
            return false;

        // 改成16进制
        const auto hexValue = [](const char c) noexcept -> int
        {
            if (c >= '0' && c <= '9') return c - '0';
            if (c >= 'a' && c <= 'f') return c - 'a' + 10;
            return -1;
        };

        AssetID id{};
        std::size_t digitIndex = 0;
        for (std::size_t i = 0; i < text.size(); ++i)
        {
            if (i == 8 || i == 13 || i == 18 || i == 23)
            {
                if (text[i] != '-') return false;
                continue;
            }

            const int digit = hexValue(text[i]);
            if (digit < 0) return false;

            //这行把新读到的一个十六进制数字
            if (digitIndex < 16)
                id.High = (id.High << 4) | static_cast<std::uint64_t>(digit);
            else
                id.Low = (id.Low << 4) | static_cast<std::uint64_t>(digit);
            ++digitIndex;
        }

        if (!id.isValid())
            return false;
        output = id;
        return true;
    }
}
