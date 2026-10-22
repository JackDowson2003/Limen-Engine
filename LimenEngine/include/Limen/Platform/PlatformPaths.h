//
// Created by chenlong on 2026/10/20.
//

#pragma once
#include <filesystem>

#include "Limen/Core/Core.h"
#include "Limen/Tool/LString.h"
namespace Limen
{
    // 成功时返回内置资源目录的规范绝对路径，并清空 error；
    // 失败时填写 error，保持 output 不变。
    [[nodiscard]] LIMEN_API bool TryGetBuiltInAssetRoot(
        std::filesystem::path& output, LString& error);
}