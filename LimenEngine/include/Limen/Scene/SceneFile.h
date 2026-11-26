//
// Created by chenlong on 2026/11/25.
//

#pragma once

#include <filesystem>

#include "Limen/Scene/SceneSerializer.h"

namespace Limen
{
    // 加载失败保留调用方原有的document
    [[nodiscard]]
    LIMEN_API bool TryLoadSceneFile(
        const std::filesystem::path& path,
        SceneDocument &outDocument,
        SceneFileError &error
    );

    // 原子性的保存文件
    [[nodiscard]]
    LIMEN_API bool TrySaveSceneFile(
      const  std::filesystem::path &path,
      const SceneDocument &document,
      SceneFileError &error
    );
}