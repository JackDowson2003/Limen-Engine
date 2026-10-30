//
// Created by chenlong on 2026/10/23.
//

#include <toml.hpp>
#include <fstream>
#include <string>
#include "Limen/Asset/AssetRegistry.h"

#include "Limen/Core/Log.h"
#include "Limen/Tool/LMap.h"

namespace Limen
{
    namespace
    {
        struct AssetIDHash
        {
            std::size_t operator()(const AssetID &id) const noexcept
            {
                const auto high = std::hash<std::uint64_t>{}(id.High);
                const auto low = std::hash<std::uint64_t>{}(id.Low);
                return high ^ (low << 1);
            }
        };

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

        bool IsStrictChildOf(const std::filesystem::path &root,
                             const std::filesystem::path &target)
        {
            auto targetPart = target.begin();
            for (const auto &rootPart: root)
            {
                if (targetPart == target.end() || *targetPart != rootPart)
                    return false;
                ++targetPart;
            }
            return targetPart != target.end();
        }

        /**
         * @brief 判断一个 std::string_view 的字节序列是否为合法的 UTF-8 编码。
         *
         * 本函数只做「语法层面」的校验，即字节序列是否符合 UTF-8 的编码规则，
         * 不涉及语义（如是否是文本、是否带 BOM 等）。
         *
         * UTF-8 编码规则回顾：
         *   码点范围           字节数   首字节       后续字节
         *   U+0000..U+007F     1       0xxxxxxx     —
         *   U+0080..U+07FF     2       110xxxxx     10xxxxxx
         *   U+0800..U+FFFF     3       1110xxxx     10xxxxxx 10xxxxxx
         *   U+10000..U+10FFFF  4       11110xxx     10xxxxxx 10xxxxxx 10xxxxxx
         *
         * 除了上述基本格式，还必须排除以下非法情况：
         *   1. 后续字节必须以 "10" 开头（范围 0x80-0xBF），否则非法。
         *   2. 过度长编码（overlong）：用多于必要字节数编码一个码点，非法。
         *      例如 0xC0/0xC1 作首字节永远非法（2 字节编码的最小码点是 U+0080）。
         *   3. 代理区码点 U+D800..U+DFFF 不允许出现在 UTF-8 中（对应 0xED 开头的特例）。
         *   4. 码点不能超过 U+10FFFF（对应 0xF4 开头的特例）。
         *   5. 首字节 0xF5..0xFF 永远非法。
         *
         * @param sv 待校验的字节序列。
         * @return true 表示整个字节序列是合法 UTF-8；false 表示存在非法编码。
         *
         * 时间复杂度 O(n)，空间复杂度 O(1)，不分配内存。
         */
        bool IsValidUtf8(const std::string_view sv)
        {
            // 以 unsigned char* 遍历，避免 char 的符号性（signedness）带来的未定义/错误行为。
            const auto *p = reinterpret_cast<const unsigned char *>(sv.data());
            const auto *end = p + sv.size();

            while (p < end)
            {
                const unsigned char c = *p; // 当前字符的首字节

                if (c <= 0x7F)
                {
                    // ---- 单字节 ASCII：0xxxxxxx ----
                    // 范围 0x00-0x7F，本身就是合法 UTF-8，直接前进 1 字节。
                    ++p;
                } else if (c >= 0xC2 && c <= 0xDF)
                {
                    // ---- 2 字节字符首字节：110xxxxx ----
                    // 注意下界是 0xC2 而非 0xC0：
                    //   0xC0、0xC1 只能表示 U+0000..U+007F 范围的码点，
                    //   属于「过度长编码」，永远非法，因此直接排除。

                    // 剩余字节不足 2 个，说明序列被截断。
                    if (end - p < 2) return false;

                    // 第 2 个字节必须形如 10xxxxxx。
                    if ((p[1] & 0xC0) != 0x80) return false;

                    p += 2;
                } else if (c >= 0xE0 && c <= 0xEF)
                {
                    // ---- 3 字节字符首字节：1110xxxx ----

                    // 剩余字节不足 3 个，序列被截断。
                    if (end - p < 3) return false;

                    // 第 2、3 个字节都必须形如 10xxxxxx。
                    if ((p[1] & 0xC0) != 0x80) return false;
                    if ((p[2] & 0xC0) != 0x80) return false;

                    // 特例 1：首字节为 0xE0 时，第 2 字节必须 >= 0xA0。
                    //   0xE0 0x80..0x9F 会编码出 U+0000..U+07FF 的码点，
                    //   而这些码点本应只用 1 或 2 字节编码，属于过度长编码，非法。
                    if (c == 0xE0 && p[1] < 0xA0) return false;

                    // 特例 2：首字节为 0xED 时，第 2 字节必须 <= 0x9F。
                    //   0xED 0xA0..0xBF 会编码出 U+D800..U+DFFF，
                    //   即 UTF-16 的代理区（surrogate），在 UTF-8 中禁止出现。
                    if (c == 0xED && p[1] > 0x9F) return false;

                    p += 3;
                } else if (c >= 0xF0 && c <= 0xF4)
                {
                    // ---- 4 字节字符首字节：11110xxx ----
                    // 上界 0xF4 而非 0xF7：
                    //   Unicode 码点最大为 U+10FFFF，对应 4 字节编码的最高首字节是 0xF4，
                    //   0xF5..0xFF 会超出范围，永远非法，因此直接排除。

                    // 剩余字节不足 4 个，序列被截断。
                    if (end - p < 4) return false;

                    // 第 2、3、4 个字节都必须形如 10xxxxxx。
                    if ((p[1] & 0xC0) != 0x80) return false;
                    if ((p[2] & 0xC0) != 0x80) return false;
                    if ((p[3] & 0xC0) != 0x80) return false;

                    // 特例 1：首字节为 0xF0 时，第 2 字节必须 >= 0x90。
                    //   0xF0 0x80..0x8F 会编码出 U+0000..U+FFFF 的码点，
                    //   这些码点本应只用 1-3 字节编码，属于过度长编码，非法。
                    if (c == 0xF0 && p[1] < 0x90) return false;

                    // 特例 2：首字节为 0xF4 时，第 2 字节必须 <= 0x8F。
                    //   0xF4 0x90..0xBF 会编码出 U+110000 及以上的码点，
                    //   超出 Unicode 最大码点 U+10FFFF，非法。
                    if (c == 0xF4 && p[1] > 0x8F) return false;

                    p += 4;
                } else
                {
                    // ---- 其余所有情况均为非法首字节 ----
                    //   0x80..0xBF：后续字节不能作为首字节出现（孤立 continuation byte）。
                    //   0xC0、0xC1：过度长编码，已在上面的分支排除，这里兜底。
                    //   0xF5..0xFF：超出 Unicode 范围或无效首字节。
                    return false;
                }
            }

            // 全部字节都成功消费完，说明整个序列合法。
            return true;
        }

        bool CheckRelativePath(const std::string_view text, std::filesystem::path &output)
        {
            // 检查原始输入；不能先规范化，否则 a/../b 会被掩盖。
            if (text.empty() || text.front() == '/' || text.back() == '/' ||
                text.find("//") != text.npos || text.find('\\') != text.npos ||
                text.find(':') != text.npos || text.find('\0') != text.npos ||
                text.find_first_of("<>\"|?*") != text.npos)
                return false;

            for (const char c: text)
                if (static_cast<unsigned char>(c) < 0x20)
                    return false;

            if (!IsValidUtf8(text))
                return false;

            // 显式把约定的 UTF-8 字节交给 filesystem，避免 Windows 本地编码歧义。
            std::u8string utf8Path;
            utf8Path.reserve(text.size());
            for (const char c: text)
                utf8Path.push_back(static_cast<char8_t>(static_cast<unsigned char>(c)));

            std::filesystem::path path(utf8Path);
            if (path.is_absolute() || path.has_root_name())
                return false;

            for (const auto &part: path)
            {
                const auto name = part.generic_u8string();
                if (name.empty() || name == u8"." || name == u8".." ||
                    name.back() == u8'.' || name.back() == u8' ' ||
                    IsWindowsReservedComponent(name))
                    return false;
            }

            output = std::move(path);
            return true;
        }

        bool TryReadSidecar(const std::filesystem::path &sidecar, AssetID &output, AssetRegistryError &error)
        {
            auto fail = [&](const AssetRegistryErrorCode code, const std::string_view message)
            {
                error = {
                    .Code = code,
                    .Path = sidecar,
                    .Message = LString(message)
                };
                return false;
            };

            std::error_code fileError;
            if (!std::filesystem::is_regular_file(sidecar, fileError))
                return fail(AssetRegistryErrorCode::FileAccess, "元数据文件不可访问或不是普通文件");

            auto parsed = toml::try_parse(sidecar, toml::spec::v(1, 0, 0));
            if (parsed.is_err())
            {
                const auto &problems = parsed.unwrap_err();
                if (problems.empty())
                    return fail(AssetRegistryErrorCode::InvalidMetadata, "TOML 解析失败");
                return fail(AssetRegistryErrorCode::InvalidMetadata, toml::format_error(problems.front()));
            }

            const auto &root = parsed.unwrap();
            if (!root.is_table())
                return fail(AssetRegistryErrorCode::InvalidMetadata, "元数据根节点必须是表");
            if (!root.contains("schema_version") ||
                !root.at("schema_version").is_integer())
            {
                return fail(AssetRegistryErrorCode::InvalidMetadata, "必须提供整数 schema_version");
            }

            if (root.at("schema_version").as_integer() != 1)
                return fail(AssetRegistryErrorCode::UnsupportedSchema, "目前只支持 schema_version = 1");

            if (!root.contains("asset_id") || !root.at("asset_id").is_string())
                return fail(AssetRegistryErrorCode::InvalidMetadata, "必须提供字符串 asset_id");

            for (const auto &entry: root.as_table())
            {
                const auto &key = entry.first;
                if (key != "schema_version" && key != "asset_id")
                    return fail(AssetRegistryErrorCode::InvalidMetadata, "元数据包含未知字段");
            }
            AssetID id;
            if (!TryParseAssetID(root.at("asset_id").as_string(), id))
            {
                return fail(AssetRegistryErrorCode::InvalidMetadata, "asset_id 不是有效的小写非零 ID");
            }
            output = id;
            error = {};
            return true;
        }

        // 把 ASCII 大写字母转成小写，方便做大小写不敏感的比较
        std::u8string FoldAscii(std::u8string value)
        {
            for (char8_t &c: value)
                if (c >= u8'A' && c <= u8'Z')
                    c = static_cast<char8_t>(c - u8'A' + u8'a');
            return value;
        }
    }

    struct AssetRegistry::Data
    {
        explicit Data(std::filesystem::path root)
            : AssetRoot(std::move(root))
        {
        }

        std::filesystem::path AssetRoot;
        LMap<AssetID, AssetRecord, AssetIDHash> ByID;
        LMap<LString, AssetID> ByPath;
        LMap<LString, AssetID> ByFoldedPath; // 检测路径大小写冲突，不用于精确查询
        bool Loaded = false;
    };

    AssetRegistry::AssetRegistry(std::filesystem::path assetRoot)
        : m_Data(CreateScope<Data>(std::move(assetRoot)))
    {
    }

    AssetRegistry::~AssetRegistry() = default;

    bool AssetRegistry::TryLoad(AssetRegistryError &error)
    {
        auto fail = [&](const AssetRegistryErrorCode code, const std::filesystem::path &path,
                        std::string_view msg) -> bool
        {
            error =
            {
                .Code = code,
                .Path = path,
                .Message = LString(msg)
            };
            return false;
        };

        std::error_code pathError;
        const auto root = std::filesystem::canonical(m_Data->AssetRoot, pathError);
        if (pathError)
            return fail(AssetRegistryErrorCode::InvalidRoot, m_Data->AssetRoot, "无法解析资产根目录");
        if (!std::filesystem::is_directory(root, pathError) || pathError)
            return fail(AssetRegistryErrorCode::InvalidRoot, m_Data->AssetRoot, "资产根不存在或不是目录");

        auto candidate = CreateScope<Data>(root);
        std::error_code scanError;
        std::filesystem::recursive_directory_iterator it(root, std::filesystem::directory_options::none, scanError);
        if (scanError)
            return fail(AssetRegistryErrorCode::FileAccess, root, "无法遍历资产根目录");

        const std::filesystem::recursive_directory_iterator end;
        for (; it != end; it.increment(scanError))
        {
            if (scanError)
                return fail(AssetRegistryErrorCode::FileAccess, root, "扫描资产目录失败");

            const auto sidecar = it->path();
            const auto filename = sidecar.filename().generic_u8string();

            if (!FoldAscii(filename).ends_with(u8".lmmeta"))
                continue;
            if (!filename.ends_with(u8".lmmeta"))
                return fail(AssetRegistryErrorCode::InvalidPath, sidecar,
                            "元数据后缀必须使用小写 .lmmeta");

            // is_regular_file 会跟随符号链接；先确认实际目标仍在资产根内。
            std::error_code linkError;
            const auto resolvedSidecar = std::filesystem::canonical(sidecar, linkError);
            if (linkError)
                return fail(AssetRegistryErrorCode::FileAccess, sidecar,
                            "无法解析元数据实际路径");
            if (!IsStrictChildOf(root, resolvedSidecar))
                return fail(AssetRegistryErrorCode::InvalidPath, sidecar,
                            "元数据符号链接越出资产根");

            // 保留磁盘上的逻辑位置，relative()会解析链接，不适合这里
            const auto relativeSideCar = sidecar.lexically_relative(root);
            if (relativeSideCar.empty() || relativeSideCar.is_absolute())
                return fail(AssetRegistryErrorCode::InvalidPath, sidecar,
                            "无法取得项目相对路径");

            auto logicalUtf8 = relativeSideCar.generic_u8string();
            logicalUtf8.resize(logicalUtf8.size() - sizeof(".lmmeta") + 1);
            const std::string logicalPath(logicalUtf8.begin(), logicalUtf8.end());

            std::filesystem::path relativeAsset;
            if (!CheckRelativePath(logicalPath, relativeAsset))
                return fail(AssetRegistryErrorCode::InvalidPath, sidecar, "元数据对应的资产路径不合法");

            if (FoldAscii(relativeAsset.filename().generic_u8string()).ends_with(u8".lmmeta"))
                return fail(AssetRegistryErrorCode::InvalidPath, sidecar,
                            ".lmmeta 不能作为源资产后缀");

            AssetID id;
            if (!TryReadSidecar(sidecar, id, error))
                return false;

            const auto assetPath = root / relativeAsset;
            const auto expectedName = assetPath.filename().generic_u8string();
            const auto foldedExpected = FoldAscii(expectedName);

            std::error_code assetError;
            std::filesystem::directory_iterator entry(assetPath.parent_path(), assetError);
            if (assetError)
                return fail(AssetRegistryErrorCode::FileAccess, assetPath, "无法枚举资产所在目录");

            bool exactPresent = false;
            const std::filesystem::directory_iterator directoryEnd;
            for (; entry != directoryEnd; entry.increment(assetError))
            {
                if (assetError)
                    return fail(AssetRegistryErrorCode::FileAccess, assetPath, "枚举资产所在目录失败");

                const auto actualName = entry->path().filename().generic_u8string();
                if (FoldAscii(actualName) != foldedExpected)
                    continue;

                if (actualName != expectedName)
                    return fail(AssetRegistryErrorCode::InvalidPath, assetPath, "资产文件名与元数据的大小写不一致");
                exactPresent = true;
            }
            if (assetError)
                return fail(AssetRegistryErrorCode::FileAccess, assetPath,
                            "枚举资产所在目录失败");
            AssetRecordStatus status = AssetRecordStatus::Missing;
            if (exactPresent)
            {
                const auto fileStatus = std::filesystem::symlink_status(assetPath, assetError);
                if (assetError)
                    return fail(AssetRegistryErrorCode::FileAccess, assetPath, "无法读取资产文件状态");
                if (fileStatus.type() != std::filesystem::file_type::regular &&
                    fileStatus.type() != std::filesystem::file_type::symlink)
                {
                    return fail(AssetRegistryErrorCode::InvalidPath, assetPath, "资产不是普通文件或文件符号链接");
                }

                const auto resolvedAsset = std::filesystem::canonical(assetPath, assetError);
                if (assetError)
                    return fail(AssetRegistryErrorCode::FileAccess, assetPath,
                                "资产文件或符号链接目标无法解析");
                if (!IsStrictChildOf(root, resolvedAsset))
                    return fail(AssetRegistryErrorCode::InvalidPath, assetPath,
                                "资产符号链接越出资产根");

                if (!std::filesystem::is_regular_file(resolvedAsset, assetError) || assetError)
                    return fail(AssetRegistryErrorCode::InvalidPath, assetPath,
                                "资产最终目标不是普通文件");
                status = AssetRecordStatus::Present;
            }
            AssetRecord record{.ID = id, .LogicalPath = LString(logicalPath), .Status = status};
            if (!candidate->ByID.try_emplace(id, record).second)
                return fail(AssetRegistryErrorCode::DuplicateID, sidecar, "重复的资产 ID");

            if (!candidate->ByPath.try_emplace(record.LogicalPath, id).second)
                return fail(AssetRegistryErrorCode::DuplicatePath, sidecar, "重复的资产路径");

            const auto foldedUtf8 = FoldAscii(relativeAsset.generic_u8string());
            const LString foldedPath(std::string(foldedUtf8.begin(), foldedUtf8.end()));

            if (!candidate->ByFoldedPath.try_emplace(foldedPath, id).second)
                return fail(AssetRegistryErrorCode::DuplicatePath, sidecar, "资产路径存在大小写冲突");
        }

        if (scanError)
            return fail(AssetRegistryErrorCode::FileAccess, root, "扫描资产目录失败");

        candidate->Loaded = true;
        m_Data.swap(candidate);
        error = {};
        return true;
    }

    bool AssetRegistry::TryRegister(std::string_view logicalPath, AssetID &output, AssetRegistryError &error)
    {
        auto fail = [&](const AssetRegistryErrorCode code,
                        const std::filesystem::path &path,
                        const std::string_view message) -> bool
        {
            error = {.Code = code, .Path = path, .Message = LString(message)};
            return false;
        };

        if (!m_Data->Loaded)
            return fail(AssetRegistryErrorCode::NotLoaded, m_Data->AssetRoot, "资产注册表尚未加载");

        std::filesystem::path relativeAsset;
        if (!CheckRelativePath(logicalPath, relativeAsset))
            return fail(AssetRegistryErrorCode::InvalidPath, m_Data->AssetRoot,
                        "资产逻辑路径不合法");

        const auto assetPath = m_Data->AssetRoot / relativeAsset;
        if (FoldAscii(relativeAsset.filename().generic_u8string()).ends_with(u8".lmmeta"))
            return fail(AssetRegistryErrorCode::InvalidPath, assetPath,
                        ".lmmeta 是元数据后缀，不能作为源资产后缀");

        const LString pathKey(logicalPath);
        if (m_Data->ByPath.contains(pathKey))
            return fail(AssetRegistryErrorCode::AlreadyRegistered, assetPath,
                        "资产已经注册");

        const auto foldedUtf8 = FoldAscii(relativeAsset.generic_u8string());
        const LString foldedKey(std::string(foldedUtf8.begin(), foldedUtf8.end()));
        if (m_Data->ByFoldedPath.contains(foldedKey))
            return fail(AssetRegistryErrorCode::DuplicatePath, assetPath,
                        "资产路径存在大小写冲突");

        std::filesystem::path current = m_Data->AssetRoot;
        for (const auto &part: relativeAsset)
        {
            const auto next = current / part;
            const bool isAsset = next == assetPath;
            const auto expected = part.generic_u8string();
            const auto foldedExpected = FoldAscii(expected);

            std::error_code fsError;
            std::filesystem::directory_iterator entry(current, fsError);
            if (fsError)
                return fail(AssetRegistryErrorCode::FileAccess, current, "无法枚举资产目录");

            bool exact = false;
            for (const std::filesystem::directory_iterator end; entry != end; entry.increment(fsError))
            {
                if (fsError)
                    return fail(AssetRegistryErrorCode::FileAccess, current, "枚举资产目录失败");

                const auto actual = entry->path().filename().generic_u8string();
                if (FoldAscii(actual) != foldedExpected)
                    continue;
                if (actual != expected)
                    return fail(AssetRegistryErrorCode::InvalidPath, next, "资产路径大小写与磁盘不一致");
                exact = true;
            }
            if (fsError)
                return fail(AssetRegistryErrorCode::FileAccess, current, "枚举资产目录失败");
            if (!exact)
                return fail(AssetRegistryErrorCode::MissingAsset, next, "源资产不存在");

            const auto type = std::filesystem::symlink_status(next, fsError).type();
            if (fsError)
                return fail(AssetRegistryErrorCode::FileAccess, next, "无法读取资产状态");

            // 扫描器不进入符号链接目录；允许它会造成注册成功、重开后丢失记录。
            if (!isAsset && type != std::filesystem::file_type::directory)
                return fail(AssetRegistryErrorCode::InvalidPath, next, "资产路径中的目录不能是符号链接");
            if (isAsset && type != std::filesystem::file_type::regular &&
                type != std::filesystem::file_type::symlink)
                return fail(AssetRegistryErrorCode::InvalidPath, next, "源资产不是普通文件");

            current = next;
        }
        std::error_code fsError;
        const auto resolvedAsset = std::filesystem::canonical(assetPath, fsError);
        if (fsError)
            return fail(AssetRegistryErrorCode::FileAccess, assetPath, "无法解析源资产");

        // 如果不是子路径或者不是普通文件就返回错误
        if (!IsStrictChildOf(m_Data->AssetRoot, resolvedAsset) ||
            !std::filesystem::is_regular_file(resolvedAsset, fsError) || fsError)
            return fail(AssetRegistryErrorCode::InvalidPath, assetPath, "源资产目标不在资产根内或不是普通文件");

        auto sidecarPath = assetPath;
        sidecarPath += ".lmmeta";

        const auto sidecarName = sidecarPath.filename().generic_u8string();
        const auto foldedSidecarName = FoldAscii(sidecarName);
        std::error_code sidecarError;
        std::filesystem::directory_iterator entry(assetPath.parent_path(), sidecarError);
        if (sidecarError)
            return fail(AssetRegistryErrorCode::FileAccess, sidecarPath, "无法枚举元数据所在目录");

        for (const std::filesystem::directory_iterator end;
             entry != end; entry.increment(sidecarError))
        {
            if (sidecarError)
                return fail(AssetRegistryErrorCode::FileAccess, sidecarPath, "枚举元数据所在目录失败");

            const auto actual = entry->path().filename().generic_u8string();
            if (actual == sidecarName)
                return fail(AssetRegistryErrorCode::AlreadyRegistered, sidecarPath, "元数据文件已存在");
            if (FoldAscii(actual) == foldedSidecarName)
                return fail(AssetRegistryErrorCode::DuplicatePath, sidecarPath, "元数据文件名存在大小写冲突");
        }
        if (sidecarError)
            return fail(AssetRegistryErrorCode::FileAccess, sidecarPath, "枚举元数据所在目录失败");

        // 磁盘提交之前完成索引分配；失败时丢弃 candidate，现有索引不变。
        auto candidate = CreateScope<Data>(*m_Data);
        AssetID id;
        do
        {
            id = GenerateAssetID();
        } while (candidate->ByID.contains(id));

        AssetRecord record{
            .ID = id,
            .LogicalPath = pathKey,
            .Status = AssetRecordStatus::Present
        };
        if (!candidate->ByID.try_emplace(id, record).second)
            return fail(AssetRegistryErrorCode::DuplicateID, sidecarPath, "生成了重复的资产 ID");
        if (!candidate->ByPath.try_emplace(pathKey, id).second ||
            !candidate->ByFoldedPath.try_emplace(foldedKey, id).second)
            return fail(AssetRegistryErrorCode::DuplicatePath, sidecarPath, "资产路径冲突");

        toml::table fields;
        fields.emplace("schema_version", toml::value(std::int64_t{1}));
        const LString idText = AssetIDToString(id);
        fields.emplace("asset_id",
                       toml::value(std::string(idText.data(), idText.size())));
        const LString text = toml::format(
            toml::value(std::move(fields)), toml::spec::v(1, 0, 0));

        auto stagingDir = sidecarPath;
        stagingDir += ".staging-";
        stagingDir += std::string(idText.data(), idText.size());

        std::error_code stageError;
        if (!std::filesystem::create_directory(stagingDir, stageError))
            return fail(AssetRegistryErrorCode::FileAccess, stagingDir,
                        "无法独占创建元数据暂存目录");

        const auto temporaryFile = stagingDir / "metadata.tmp";
        auto cleanupStaging = [&]() -> bool
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporaryFile, cleanupError);
            if (cleanupError)
                return false;
            const bool removedDir = std::filesystem::remove(stagingDir, cleanupError);
            return removedDir && !cleanupError;
        };

        bool written = false;
        {
            std::ofstream file(temporaryFile, std::ios::binary | std::ios::trunc);
            if (file)
            {
                file.write(text.data(), static_cast<std::streamsize>(text.size()));
                file.close();
                written = static_cast<bool>(file);
            }
        }
        if (!written)
        {
            if (!cleanupStaging())
                return fail(AssetRegistryErrorCode::FileAccess, stagingDir,
                            "临时元数据写入失败，且暂存目录清理失败");
            return fail(AssetRegistryErrorCode::FileAccess, temporaryFile,
                        "临时元数据写入失败");
        }

        std::error_code commitError;
        // 与暂存文件在同一文件系统；目标已存在时失败，不会覆盖 sidecar。
        std::filesystem::create_hard_link(temporaryFile, sidecarPath, commitError);
        if (commitError)
        {
            if (!cleanupStaging())
                return fail(AssetRegistryErrorCode::FileAccess, stagingDir,
                            "元数据提交失败，且暂存目录清理失败");

            if (commitError == std::errc::file_exists)
                return fail(AssetRegistryErrorCode::AlreadyRegistered, sidecarPath,
                            "元数据文件已存在");
            return fail(AssetRegistryErrorCode::FileAccess, sidecarPath,
                        "无法无覆盖地提交元数据文件");
        }

        // 从这里开始，sidecar 已持久存在，必须同步发布对应索引。
        m_Data.swap(candidate);
        output = id;
        error = {};

        if (!cleanupStaging())
        {
            // sidecar 和索引已提交；清理失败只记录警告，不能改报注册失败。
            if (const auto logger = Log::GetCoreLogger())
                logger->warn("AssetRegistry: sidecar 已提交，但暂存目录清理失败");
        }
        return true;
    }

    bool AssetRegistry::TryFind(const AssetID id, AssetRecord &output) const
    {
        if (!m_Data->Loaded)
            return false;
        const auto &it = m_Data->ByID.find(id);
        if (it == m_Data->ByID.end())
            return false;
        output = it->second;
        return true;
    }

    bool AssetRegistry::TryFindID(std::string_view logicalPath, AssetID &output) const
    {
        if (!m_Data->Loaded)
            return false;
        const auto it = m_Data->ByPath.find(LString(logicalPath));
        if (it == m_Data->ByPath.end())
            return false;
        output = it->second;
        return true;
    }
}
