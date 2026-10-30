//
// Created by chenlong on 2026/10/23.
//

/// 项目级、CPU-only 的公共契约，定义记录、查询和错误语义

#pragma once

#include <filesystem>
#include <string_view>

#include "Limen/Asset/AssetID.h"
#include "Limen/Core/Core.h"
#include "Limen/Tool/LString.h"

namespace Limen
{
    enum class AssetRecordStatus
    {
        Present,
        Missing
    };

    struct AssetRecord
    {
        AssetID ID{};
        LString LogicalPath; // 相对资产根，使用 / 分隔
        AssetRecordStatus Status = AssetRecordStatus::Missing;
    };

    enum class AssetRegistryErrorCode
    {
        None, NotLoaded, InvalidRoot, InvalidPath, MissingAsset,
        AlreadyRegistered, FileAccess, InvalidMetadata,
        UnsupportedSchema, DuplicateID, DuplicatePath
    };

    struct AssetRegistryError
    {
        AssetRegistryErrorCode Code = AssetRegistryErrorCode::None;
        std::filesystem::path Path;
        LString Message;
    };

    class LIMEN_API AssetRegistry final
    {
    public:
        explicit AssetRegistry(std::filesystem::path assetRoot);
        ~AssetRegistry(); // Data 不完整，析构函数必须在 .cpp 定义。

        AssetRegistry(const AssetRegistry&) = delete;
        AssetRegistry& operator=(const AssetRegistry&) = delete;

        [[nodiscard]] bool TryLoad(AssetRegistryError& error);
        [[nodiscard]] bool TryRegister(std::string_view logicalPath,
                                       AssetID& output, AssetRegistryError& error);
        [[nodiscard]] bool TryFind(AssetID id, AssetRecord& output) const;
        [[nodiscard]] bool TryFindID(std::string_view logicalPath, AssetID& output) const;

    private:
        struct Data;
        Scope<Data> m_Data; // 独占两个索引；实现细节不进入公共头。
    };

}