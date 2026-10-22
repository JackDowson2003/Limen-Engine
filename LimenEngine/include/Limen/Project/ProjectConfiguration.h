//
// Created by chenlong on 2026/10/19.
//

#pragma once

#include <cstdint>
#include <filesystem>

#include "Limen/Core/Core.h"
#include "Limen/Tool/LString.h"

namespace Limen
{
    enum class ProjectConfigurationErrorCode
    {
        None,
        FileAccess,
        InvalidToml,
        UnsupportedSchema,
        InvalidField,
        InvalidPath
    };

    struct ProjectConfigurationError
    {
        ProjectConfigurationErrorCode Code = ProjectConfigurationErrorCode::None;
        std::filesystem::path File;
        LString Key; // 整个文件出错时为空；字段出错时填写键名。
        LString Message;
    };

    struct ProjectConfiguration
    {
        std::uint32_t SchemaVersion = 0;
        LString ProjectID;
        LString DisplayName;
        std::filesystem::path ProjectFile; // 规范化后的绝对路径。
        std::filesystem::path AssetRootRelative; // 文件中保存的相对路径。
        std::filesystem::path AssetRootAbsolute; // 相对 ProjectFile 所在目录解析。
    };

    // 失败时不修改 output；不创建窗口、不初始化 GPU，也不修改全局项目状态。
    [[nodiscard]] LIMEN_API bool TryLoadProjectConfiguration(
        const std::filesystem::path &projectFile,
        ProjectConfiguration &output,
        ProjectConfigurationError &error
    );

    /**
     * 创建项目，并能用现有加载器重新打开
     *
     * 第一版把 projectDirectory 定义为尚不存在的新目录，其父目录须已存在。成功时创建 <目录名>.lmproject 和 assets/，
     * 生成一次非零 UUID 项目 ID，写入当前加载器支持的四个字段，再调用 TryLoadProjectConfiguration 回读，
     * 将规范化结果赋给 output。序列化显示名时要正确转义 TOML 字符串，不能直接拼接用户输入。先不要生成尚无加载器的 DefaultGame.toml。
     */
    [[nodiscard]] LIMEN_API bool TryCreateProjectConfiguration(
        const std::filesystem::path &projectDirectory,
        const LString &displayName,
        ProjectConfiguration &output,
        ProjectConfigurationError &error
    );
}
