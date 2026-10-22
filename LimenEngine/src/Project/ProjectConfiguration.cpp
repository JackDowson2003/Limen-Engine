//
// Created by chenlong on 2026/10/19.
//
#include <toml.hpp>
#include <string_view>
#include <utility>
#include <random>
#include <fstream>
#include "Limen/Project/ProjectConfiguration.h"

#include "Limen/Core/Log.h"

namespace
{
    // 检查单个路径分量是否为 Windows 保留设备名。
    // 扩展名不改变保留性（如 CON.txt）；其他路径命名规则由调用处校验。
    bool IsWindowsReservedComponent(std::u8string_view component)
    {
        // CON.txt 仍是保留名，只检查第一个点之前的部分
        const auto baseView = component.substr(0, component.find(u8'.'));
        std::u8string base(baseView.begin(), baseView.end());

        for (char8_t &c: base)
            if (c >= u8'a' && c <= u8'z')
                // ASCII 是从 A to a的
                c = static_cast<char8_t>(c - u8'a' + u8'A');

        if (base == u8"CON" || base == u8"PRN" ||
            base == u8"AUX" || base == u8"NUL")
            return true;

        if (base.size() == 4 &&
            (base.starts_with(u8"COM") || base.starts_with(u8"LPT")) &&
            base[3] >= u8'1' && base[3] <= u8'9')
            return true;
        return base == u8"COM¹" || base == u8"COM²" || base == u8"COM³" ||
               base == u8"LPT¹" || base == u8"LPT²" || base == u8"LPT³";
    }

    Limen::LString GenerateProjectID()
    {
        std::random_device source;
        // 建立一个uniform分布
        std::uniform_int_distribution<unsigned int> byte(0, 255);
        unsigned char bytes[16];

        for (unsigned char &value: bytes)
            value = static_cast<unsigned char>(byte(source));

        bytes[6] = static_cast<unsigned char>((bytes[6] & 0x0f) | 0x40); // UUID v4
        bytes[8] = static_cast<unsigned char>((bytes[8] & 0x3f) | 0x80); // variant

        //使用很少在栈中存放即可
        const char hex[] = "0123456789abcdef";
        Limen::LString id;
        id.reserve(36);

        for (std::size_t i = 0; i < 16; ++i)
        {
            if (i == 4 || i == 6 || i == 8 || i == 10)
                id.push_back('-');
            id.push_back(hex[bytes[i] >> 4]);
            id.push_back(hex[bytes[i] & 0x0f]);
        }

        return id;
    }
}

namespace Limen
{
    bool TryLoadProjectConfiguration(const std::filesystem::path &projectFile, ProjectConfiguration &output,
                                     ProjectConfigurationError &error)
    {
        if (projectFile.extension() != ".lmproject")
        {
            error =
            {
                .Code = ProjectConfigurationErrorCode::InvalidPath,
                .File = projectFile,
                .Key = "",
                .Message = "项目只支持 以.lmproject的方式进行加载"
            };
            return false;
        }

        if (std::error_code fileError; !std::filesystem::is_regular_file(projectFile, fileError))
        {
            error = {
                .Code = ProjectConfigurationErrorCode::FileAccess,
                .File = projectFile,
                .Key = "",
                .Message = "项目文件不存在或不是普通文件"
            };
            return false;
        }

        try
        {
            ProjectConfiguration candidate;
            auto parsed = toml::try_parse(projectFile, toml::spec::v(1, 0, 0));
            if (parsed.is_err())
            {
                const auto &problems = parsed.unwrap_err();
                error =
                {
                    .Code = ProjectConfigurationErrorCode::InvalidToml,
                    .File = projectFile,
                    .Key = "",
                    .Message = problems.empty()
                                   ? LString("TOML 解析失败")
                                   : LString(toml::format_error(problems.front()))
                };
                return false;
            }

            const auto &root = parsed.unwrap();
            if (!root.is_table() || !root.contains("schema_version") || !root.at("schema_version").is_integer())
            {
                error =
                {
                    .Code = ProjectConfigurationErrorCode::InvalidField,
                    .File = projectFile,
                    .Key = "schema_version",
                    .Message = "必须提供整数 schema_version"
                };
                return false;
            }

            if (root.at("schema_version").as_integer() != 1)
            {
                error =
                {
                    .Code = ProjectConfigurationErrorCode::UnsupportedSchema,
                    .File = projectFile,
                    .Key = "schema_version",
                    .Message = "目前只支持 schema_version = 1"
                };
                return false;
            }
            candidate.SchemaVersion = 1;

            // TOML 根表类似一个 Key -> value的映射，先确认字段存在且类型正确，再取值
            auto readRequiredString = [&](const char *key, LString &target) -> bool
            {
                if (!root.contains(key) || !root.at(key).is_string() ||
                    root.at(key).as_string().empty())
                {
                    error = {
                        .Code = ProjectConfigurationErrorCode::InvalidField,
                        .File = projectFile,
                        .Key = key,
                        .Message = "必填字段必须是非空字符串"
                    };
                    return false;
                }

                target = LString(root.at(key).as_string());
                return true;
            };

            if (!readRequiredString("project_id", candidate.ProjectID))
                return false;

            LString assetRootText;
            if (!readRequiredString("asset_root", assetRootText))
                return false;

            // display_name 可选；出现了就必须是非空字符串。
            if (root.contains("display_name"))
            {
                const auto &name = root.at("display_name");
                if (!name.is_string() || name.as_string().empty())
                {
                    error = {
                        .Code = ProjectConfigurationErrorCode::InvalidField,
                        .File = projectFile,
                        .Key = "display_name",
                        .Message = "display_name 必须是非空字符串"
                    };
                    return false;
                }
                candidate.DisplayName = LString(name.as_string());
            }
            // 根表只允许约定的字段；否则拼错字段名也会被悄悄忽略。
            for (const auto &entry: root.as_table())
            {
                const std::string &key = entry.first;
                if (key != "schema_version" && key != "project_id" &&
                    key != "display_name" && key != "asset_root")
                {
                    error = {
                        .Code = ProjectConfigurationErrorCode::InvalidField,
                        .File = projectFile,
                        .Key = LString(key),
                        .Message = "未知的项目配置字段"
                    };
                    return false;
                }
            }

            // project_id 使用 8-4-4-4-12 的 UUID 文本形态，并拒绝全零 ID
            bool validID = candidate.ProjectID.size() == 36;
            bool hasNonZeroDigit = false;

            // 遍历 chars
            for (std::size_t i = 0; validID && i < candidate.ProjectID.size(); ++i)
            {
                const char c = candidate.ProjectID[i];

                if (i == 8 || i == 13 || i == 18 || i == 23)
                {
                    validID = c == '-';
                    continue;
                }

                const bool hexDigit =
                        (c >= '0' && c <= '9') ||
                        (c >= 'a' && c <= 'f') ||
                        (c >= 'A' && c <= 'F');
                validID = hexDigit;
                hasNonZeroDigit |= c != '0';
            }

            if (!validID || !hasNonZeroDigit)
            {
                error = {
                    .Code = ProjectConfigurationErrorCode::InvalidField,
                    .File = projectFile,
                    .Key = "project_id",
                    .Message = "project_id 必须是非全零的 UUID"
                };
                return false;
            }

            auto invalidRoot = [&](const char *msg) -> bool
            {
                error =
                {
                    .Code = ProjectConfigurationErrorCode::InvalidPath,
                    .File = projectFile,
                    .Key = "asset_root",
                    .Message = msg
                };
                return false;
            };
            // TOML 中约定用 / 分隔；先检查原始文本，不能先规范化再检查 ..。
            const std::string_view text(assetRootText.data(), assetRootText.size());
            if (text.empty() || text.front() == '/' || text.back() == '/' ||
                text.find("//") != text.npos || text.find('\\') != text.npos ||
                text.find(':') != text.npos || text.find('\0') != text.npos ||
                text.find_first_of("<>\"|?*") != text.npos)
            {
                return invalidRoot("asset_root 必须是项目内的相对路径");
            }

            for (const char c: text)
            {
                if (static_cast<unsigned char>(c) < 0x20)
                    return invalidRoot("asset_root 含有控制字符");
            }

            // TOML 字符串是 UTF-8；显式以 UTF-8 构造 path，避免 Windows 窄字符编码歧义
            std::u8string utf8Path;
            utf8Path.reserve(text.size());
            for (const char c: text)
                utf8Path.push_back(static_cast<unsigned char>(c));

            const std::filesystem::path relativeRoot(utf8Path);
            if (relativeRoot.is_absolute() || relativeRoot.has_root_name())
                return invalidRoot("asset_root 不能是绝对路径");

            for (const auto &part: relativeRoot)
            {
                if (part == "." || part == "..")
                    return invalidRoot("asset_root 不能包含 . 或 ..");

                if (const auto name = part.generic_u8string();
                    name.empty() || name.back() == u8'.' || name.back() == u8' ' || IsWindowsReservedComponent(name)
                )
                    return invalidRoot("asset_root 含有不可移植的目录名");
            }

            std::error_code pathError;
            candidate.ProjectFile = std::filesystem::canonical(projectFile, pathError);
            if (pathError)
            {
                error =
                {
                    .Code = ProjectConfigurationErrorCode::FileAccess,
                    .File = projectFile, .Key = "", .Message = "无法解析项目文件路径"
                };
                return false;
            }

            const auto &projectRoot = candidate.ProjectFile.parent_path();
            const auto &resolvedRoot = std::filesystem::canonical(projectRoot / relativeRoot, pathError);

            if (pathError || !std::filesystem::is_directory(resolvedRoot, pathError))
                return invalidRoot("asset_root 不存在或不是目录");

            // canonical 会解析符号链接；按路径分量比较，不能用字符串前缀比较
            auto projectPart = projectRoot.begin();
            auto assetPart = resolvedRoot.begin();
            for (; projectPart != projectRoot.end(); ++projectPart, ++assetPart)
            {
                if (assetPart == resolvedRoot.end() || *projectPart != *assetPart)
                    return invalidRoot("asset_root 路径安全校验失败, 经符号链接解析后越出了项目目录");
            }

            candidate.AssetRootRelative = relativeRoot;
            candidate.AssetRootAbsolute = resolvedRoot;

            // canonical 校验最终位置；目录枚举校验配置中每一级的实际拼写
            std::filesystem::path currentDirectory = projectRoot;
            for (const auto &part: relativeRoot)
            {
                const auto &expectedName = part.generic_u8string();
                std::error_code scanError;
                std::filesystem::directory_iterator entry(currentDirectory, scanError);

                bool exactMatch = false;
                for (const std::filesystem::directory_iterator end;
                     !scanError && entry != end; entry.increment(scanError)
                )
                {
                    if (entry->path().filename().generic_u8string() == expectedName)
                    {
                        exactMatch = true;
                        break;
                    }
                }
                if (scanError)
                {
                    error =
                    {
                        .Code = ProjectConfigurationErrorCode::FileAccess,
                        .File = projectFile,
                        .Key = "asset_root",
                        .Message = "无法枚举资源目录"
                    };
                    return false;
                }
                if (!exactMatch)
                    return invalidRoot("asset_root 与实际目录名的拼写不一致");

                currentDirectory /= part;
            }

            if (candidate.DisplayName.empty())
            {
                const auto stem = candidate.ProjectFile.stem().u8string();
                candidate.DisplayName = LString(std::string(stem.begin(), stem.end()));
            }

            // 直到此处才提交：上面任何失败都不会改变 output。
            output = std::move(candidate);
            error = {};
            return true;
        } catch (const std::ios_base::failure &expectation)
        {
            error =
            {
                .Code = ProjectConfigurationErrorCode::FileAccess,
                .File = projectFile,
                .Key = "",
                .Message = expectation.what()
            };
            return false;
        }
    }

    bool TryCreateProjectConfiguration(const std::filesystem::path &projectDirectory, const LString &displayName,
                                       ProjectConfiguration &output, ProjectConfigurationError &error)
    {
        namespace fs = std::filesystem;
        using Code = ProjectConfigurationErrorCode;

        fs::path root;
        fs::path assets;
        fs::path projectFile;
        fs::path temporaryFile;
        bool createdRoot = false;
        bool createdAssets = false;
        bool createdTemporary = false;
        bool createdProjectFile = false;

        // 只删除本次创建的项目内容，remove 不会地柜删除别人放入的文件
        struct RollbackFailure
        {
            fs::path Path;
            std::error_code Error;
        };

        // 只删除本次创建的内容；不用 remove_all，以免删除后来放入的文件。
        auto rollback = [&]() -> RollbackFailure
        {
            RollbackFailure firstFailure;

            auto removeCreated = [&](bool created, const fs::path &path)
            {
                if (!created)
                    return;

                std::error_code removeError;
                fs::remove(path, removeError);
                if (removeError && !firstFailure.Error)
                {
                    firstFailure.Path = path;
                    firstFailure.Error = removeError;
                }
            };

            removeCreated(createdTemporary, temporaryFile);
            removeCreated(createdProjectFile, projectFile);
            removeCreated(createdAssets, assets);
            removeCreated(createdRoot, root);
            return firstFailure;
        };

        auto reportRollbackFailure = [&](const RollbackFailure &failure)
        {
            if (!failure.Error)
                return;

            const std::string reason = failure.Error.message();
            error.Message += "；回滚失败，可能残留项目内容：";
            error.Message += reason.c_str();
            error.Code = Code::FileAccess;
            error.File = failure.Path;
        };

        auto fail = [&](const Code code, const char *key, const char *msg)
        {
            const auto cleanup = rollback();
            error = {
                .Code = code,
                .File = projectFile.empty() ? projectDirectory : projectFile,
                .Key = key,
                .Message = msg
            };
            reportRollbackFailure(cleanup);
            return false;
        };

        try
        {
            if (displayName.empty())
                return fail(Code::InvalidField, "display_name", "项目名不能为空");

            if (!projectDirectory.is_absolute())
                return fail(Code::InvalidPath, "", "项目目录必须是绝对路径");

            // 获取文件名
            const std::u8string name = projectDirectory.filename().generic_u8string();

            if (name.empty() || name == u8"." || name == u8".." ||
                name.back() == u8'.' || name.back() == u8' ' ||
                IsWindowsReservedComponent(name))
            {
                return fail(Code::InvalidPath, "", "项目目录名不可移植");
            }

            for (const char8_t c: name)
            {
                if (c < u8' ' || c == u8'<' || c == u8'>' || c == u8':' ||
                    c == u8'"' || c == u8'|' || c == u8'?' || c == u8'*' ||
                    c == u8'\\')
                {
                    return fail(Code::InvalidPath, "", "项目目录名含有不可移植字符");
                }
            }

            std::error_code fsError;
            const auto parent = fs::canonical(projectDirectory.parent_path(), fsError);

            if (fsError || !fs::is_directory(parent, fsError))
                return fail(Code::InvalidPath, "", "项目父目录不存在或不可访问");

            root = parent / fs::path(name);
            assets = root / "assets";

            std::u8string filename = name;
            filename += u8".lmproject";
            projectFile = root / fs::path(filename);
            temporaryFile = projectFile;
            temporaryFile += ".tmp";

            // 仅接受全新目录。false 包括目录已存在；绝不覆盖它
            if (!fs::create_directory(root, fsError))
                return fail(Code::FileAccess, "", "目标项目目录已存在或无法创建");
            createdRoot = true;

            if (!fs::create_directory(assets, fsError))
                return fail(Code::FileAccess, "", "无法创建项目资产目录");
            createdAssets = true;

            toml::table fields;
            fields.emplace("schema_version", toml::value(std::int64_t{1}));
            const LString projectID = GenerateProjectID();
            fields.emplace("project_id", toml::value(std::string(projectID.data(), projectID.size())));
            fields.emplace("display_name", toml::value(std::string(displayName.data(), displayName.size())));
            fields.emplace("asset_root", toml::value(std::string("assets")));

            // toml11 负责引号、换行等转意，text 是文件内容，不是文件名
            const LString text = toml::format(toml::value(std::move(fields)), toml::spec::v(1, 0, 0));
            createdTemporary = true;

            bool written = false;
            {
                if (std::ofstream file(temporaryFile, std::ios::binary | std::ios::trunc); file)
                {
                    file.write(text.data(), static_cast<std::streamsize>(text.size()));
                    file.close();
                    written = static_cast<bool>(file);
                }
            }

            if (!written)
                return fail(Code::FileAccess, "", "项目配置临时文件写入失败");

            fs::rename(temporaryFile, projectFile, fsError);
            if (fsError)
                return fail(Code::FileAccess, "", "无法提交项目配置文件");

            createdTemporary = false;
            createdProjectFile = true;

            // 使用同一个加载器回读，通过后修改调用方的output
            ProjectConfiguration candidate;
            if (!TryLoadProjectConfiguration(projectFile, candidate, error))
            {
                const auto cleanup = rollback();
                reportRollbackFailure(cleanup);
                return false;
            }

            output = std::move(candidate);
            error = {};
            return true;
        } catch (const std::exception &exception)
        {
            return fail(Code::FileAccess, "", exception.what());
        }
    }
}
