#include "Project/EditorStartupScene.h"

#include <fstream>
#include <string>
#include <string_view>
#include <system_error>
#include <utility>

#include <toml.hpp>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Project/ProjectConfiguration.h"

namespace
{
    bool IsMissing(const std::filesystem::file_status status, const std::error_code &error)
    {
        return status.type() == std::filesystem::file_type::not_found &&
               (!error || error == std::errc::no_such_file_or_directory);
    }

    bool IsInsideProject(const std::filesystem::path &projectRoot, const std::filesystem::path &resolved)
    {
        const auto relative = resolved.lexically_relative(projectRoot);
        return !relative.empty() && !relative.is_absolute() && *relative.begin() != "..";
    }
}

namespace LimenEditor
{
    bool TryResolveEditorStartupScene(
        const Limen::ProjectConfiguration &project,
        const Limen::AssetRegistry &registry,
        std::filesystem::path &output,
        Limen::ProjectConfigurationError &error)
    {
        namespace fs = std::filesystem;
        using Code = Limen::ProjectConfigurationErrorCode;

        constexpr auto sceneKey = "startup_scene_asset_id";
        constexpr auto fullSceneKey = "game.startup_scene_asset_id";
        const auto projectRoot = project.ProjectFile.parent_path();
        const auto configDirectory = projectRoot / "Config";
        const auto configFile = configDirectory / "DefaultGame.toml";
        const auto fail = [&](const Code code, const std::string_view key, const std::string_view message)
        {
            error = {
                .Code = code,
                .File = configFile,
                .Key = Limen::LString(key),
                .Message = Limen::LString(message)
            };
            return false;
        };
        const auto noStartupScene = [&]()
        {
            output.clear();
            error = {};
            return true;
        };

        // 单独检查 Config，避免把断开的目录链接当成未配置。
        std::error_code fsError;
        const auto directoryStatus = fs::symlink_status(configDirectory, fsError);
        if (IsMissing(directoryStatus, fsError))
            return noStartupScene();
        if (fsError)
            return fail(Code::FileAccess, {}, "无法读取 Config 目录状态");
        const auto resolvedDirectory = fs::canonical(configDirectory, fsError);
        if (fsError || !fs::is_directory(resolvedDirectory, fsError))
            return fail(Code::FileAccess, {}, "Config 路径不可访问或不是目录");
        if (!IsInsideProject(projectRoot, resolvedDirectory))
            return fail(Code::InvalidPath, {}, "Config 目录链接越出项目目录");

        const auto fileStatus = fs::symlink_status(configFile, fsError);
        if (IsMissing(fileStatus, fsError))
            return noStartupScene();
        if (fsError)
            return fail(Code::FileAccess, {}, "无法读取 DefaultGame.toml 文件状态");
        const auto resolvedFile = fs::canonical(configFile, fsError);
        if (fsError || !fs::is_regular_file(resolvedFile, fsError))
            return fail(Code::FileAccess, {}, "DefaultGame.toml 不可访问或不是普通文件");
        if (!IsInsideProject(projectRoot, resolvedFile))
            return fail(Code::InvalidPath, {}, "DefaultGame.toml 链接越出项目目录");

        try
        {
            std::ifstream input(resolvedFile, std::ios::binary);
            if (!input)
                return fail(Code::FileAccess, {}, "无法打开 DefaultGame.toml");
            input.exceptions(std::ios::badbit | std::ios::failbit);
            const auto parsed = toml::try_parse(input, "DefaultGame.toml", toml::spec::v(1, 0, 0));
            if (parsed.is_err())
            {
                const auto &problems = parsed.unwrap_err();
                return fail(Code::InvalidToml, {},
                            problems.empty() ? "TOML 解析失败" : toml::format_error(problems.front()));
            }

            const auto &root = parsed.unwrap();
            if (!root.contains("schema_version") || !root.at("schema_version").is_integer())
                return fail(Code::InvalidField, "schema_version", "必须提供整数 schema_version");
            if (root.at("schema_version").as_integer() != 1)
                return fail(Code::UnsupportedSchema, "schema_version", "目前只支持 schema_version = 1");

            if (!root.contains("game"))
                return noStartupScene();
            const auto &game = root.at("game");
            if (!game.is_table())
                return fail(Code::InvalidField, "game", "game 必须是 TOML 表");
            if (!game.contains(sceneKey))
                return noStartupScene();

            Limen::AssetID id;
            const auto &sceneValue = game.at(sceneKey);
            if (!sceneValue.is_string() || !Limen::TryParseAssetID(sceneValue.as_string(), id))
                return fail(Code::InvalidField, fullSceneKey, "启动场景必须是有效的非零 AssetID 字符串");

            Limen::AssetRecord record;
            if (!registry.TryFind(id, record))
                return fail(Code::InvalidField, fullSceneKey, "启动场景 AssetID 未在项目资产注册表中登记");
            if (record.Status != Limen::AssetRecordStatus::Present)
                return fail(Code::FileAccess, fullSceneKey, "启动场景资产文件不存在");

            // 注册表已经验证了逻辑路径，按 UTF-8 解释后拼至已规范化的资产根。
            const std::u8string logicalPath(record.LogicalPath.begin(), record.LogicalPath.end());
            auto scenePath = project.AssetRootAbsolute / fs::path(logicalPath);
            if (scenePath.extension() != ".lscene")
                return fail(Code::InvalidField, fullSceneKey, "启动场景资产必须是 .lscene 文件");

            output = std::move(scenePath);
            error = {};
            return true;
        }
        catch (const std::ios_base::failure &failure)
        {
            return fail(Code::FileAccess, {}, failure.what());
        }
    }
}
