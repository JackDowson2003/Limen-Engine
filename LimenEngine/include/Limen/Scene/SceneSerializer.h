//
// Created by chenlong on 2026/11/25.
//

#pragma once

#include <filesystem>
#include <string_view>
#include "Limen/Scene/SceneDocument.h"

// strenuous adj: 费力的

namespace Limen
{
    // 场景文件的错误码 类似.umap使用的
    enum class SceneFileErrorCode
    {
        None,
        UnsupportedSchema,
        InvalidToml,
        InvalidField,
        // 文件访问也会出错
        FileAccess,
        InvalidID,
        DuplicateID,
        // 写入错误
        WriteFailed,
        // 原子替换错误
        AtomicReplaceFailed,
        // 实例化失败
        InstantiatedFailed
    };

    struct SceneFileError
    {
        SceneFileErrorCode Code = SceneFileErrorCode::None;
        LString Key;
        LString Message;
        std::filesystem::path File{};

    };

    // 检查场景文档的数据，失败时填写具体的错误字段
    [[nodiscard]]
    LIMEN_API bool ValidateSceneDocument(
        const SceneDocument &document,
        SceneFileError &error
    );

    // 校验通过并生成完整信息的文本后，才替换outText
    [[nodiscard]]
    LIMEN_API bool TrySerializeSceneDocument(
        const SceneDocument& document,
        LString &outText,
        SceneFileError& error
    );

    [[nodiscard]]
    LIMEN_API bool TryDeserializeSceneDocument(
        std::string_view text,
        SceneDocument &outDocument,
        SceneFileError& error
    );
}