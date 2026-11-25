//
// Created by chenlong on 2026/11/25.
//

#pragma once

#include <string_view>
#include "Limen/Scene/SceneDocument.h"

namespace Limen
{
    // 场景文件的错误码 类似.umap使用的
    enum class SceneFileErrorCode
    {
        None,
        UnsupportedSchema,
        InvalidToml,
        InvalidField,
        InvalidID,
        DuplicateID
    };

    struct SceneFileError
    {
        SceneFileErrorCode Code = SceneFileErrorCode::None;
        LString Key;
        LString Message;
    };

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