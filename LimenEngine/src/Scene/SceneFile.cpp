//
// Created by chenlong on 2026/11/25.
//

#include "Limen/Scene/SceneFile.h"

#include <cerrno>
#include <chrono>
#include <cstdio>
#include <string>
#include <utility>
#include <fstream>
#include <string_view>
#include <system_error>

namespace Limen
{
    namespace
    {
        /**
         * 仅打开二进制文件
         * @param path 文件路径
         * @param openError 打开后的错误 有则填 无则正常
         * @return 文件标识符
         */
        std::FILE *OpenExclusiveBinaryFile(const std::filesystem::path &path, int &openError)
        {
#ifdef LIMEN_PLATFORM_WINDOWS
            std::FILE *file = nullptr;
            openError = _wfopen_s(&file, path.c_str(), L"wbx");
#else
            std::FILE *file = std::fopen(path.c_str(), "wbx");
            openError = file ? 0 : errno;
#endif
            return file;
        }

        // 在目标文件的同一目录生成候选名字，只有遇到同名文件时才换个名字重试
        std::FILE *CreateSceneTemporaryFile(const std::filesystem::path &path,
                                            std::filesystem::path &temporaryPath,
                                            int &openError)
        {
            const auto directory = path.parent_path();
            // 获取时间戳,以时钟周期来获取
            const auto stamp = std::chrono::steady_clock::now().time_since_epoch().count();

            constexpr int kMaxAttempts = 32;
            for (int attempt = 0; attempt < kMaxAttempts; ++attempt)
            {
                const auto name = ".limen-scene-" + std::to_string(stamp) +
                                  "-" + std::to_string(attempt) + ".tmp";

                auto candidate = directory / name;

                // 独占打开后，才把临时文件交给管理方
                if (auto *file = OpenExclusiveBinaryFile(candidate, openError))
                {
                    temporaryPath = std::move(candidate);
                    return file;
                }

                if (openError != EEXIST)
                    return nullptr;
            }
            return nullptr;
        }
    }

    bool TryLoadSceneFile(const std::filesystem::path &path, SceneDocument &outDocument, SceneFileError &error)
    {
        const auto fail = [&error, &path](std::string_view message)
        {
            error = {
                .Code = SceneFileErrorCode::FileAccess,
                .Key = {},
                .Message = LString(message),
                .File = path
            };
            return false;
        };

        if (path.extension() != ".lscene")
            return fail("场景文件扩展名必须是 .lscene");

        std::error_code statusError;
        if (!std::filesystem::is_regular_file(path, statusError))
        {
            if (statusError)
                return fail(statusError.message());

            return fail("场景文件不是普通文件");
        }

        std::ifstream input(path, std::ios::binary);
        if (!input)
            return fail("无法打开场景文件");

        LString text;
        char buffer[4096];
        while (input)
        {
            input.read(buffer, sizeof(buffer));
            // 每次追加的字节数
            const auto count = input.gcount();
            if (count > 0)
                text.append(buffer, static_cast<LString::size_type>(count));
        }

        // 最后的字节可能不满4096 Bytes, 正常到达EOF 时仍要保留这些字节
        if (input.bad() || !input.eof())
            return fail("读取场景文件失败");

        if (!TryDeserializeSceneDocument(text, outDocument, error))
        {
            error.File = path;
            return false;
        }

        return true;
    }

    bool TrySaveSceneFile(const std::filesystem::path &path, const SceneDocument &document, SceneFileError &error)
    {
        using Code = SceneFileErrorCode;

        const auto fail = [&error, &path](Code code, const std::string_view message)
        {
            error = {
                .Code = code,
                .Key = {},
                .Message = LString(message),
                .File = path
            };
            return false;
        };

        if (path.extension() != ".lscene")
        {
            return fail(Code::FileAccess, "文件后缀名不是.lscene");
        }

        LString text;
        // 先进行序列化
        if (!TrySerializeSceneDocument(document, text, error))
        {
            error.File = path;
            return false;
        }

        std::error_code pathError;
        const auto target = std::filesystem::absolute(path, pathError);
        if (pathError)
            return fail(Code::FileAccess, pathError.message());

        std::filesystem::path temporaryPath;
        int openError = 0;
        auto *file = CreateSceneTemporaryFile(target, temporaryPath, openError);

        if (!file)
        {
            const auto cause = std::error_code(openError, std::generic_category());
            return fail(Code::FileAccess, "无法创建临时创建文件" + cause.message());
        }

        const auto failAfterWrite = [&fail, &temporaryPath](
            const Code code, const std::string_view message, const std::error_code cause = {})
        {
            std::error_code cleanupError;
            std::filesystem::remove(temporaryPath, cleanupError);

            std::string detail(message);
            if (cause)
                detail += "，" + cause.message();
            if (cleanupError)
                detail += "，临时文件清理失败，" + cleanupError.message();

            return fail(code, detail);
        };

        // 分开记录结果
        const bool written = std::fwrite(text.data(), 1, text.size(),file) == text.size();
        const bool flushed = std::fflush(file) == 0;
        const bool closed = std::fclose(file) == 0;
        if (!written || !flushed || !closed)
            return failAfterWrite(Code::WriteFailed, "无法完整写入临时场景文件");

        std::error_code replaceError;
        std::filesystem::rename(temporaryPath, target,replaceError);
        if (replaceError)
            return failAfterWrite(Code::AtomicReplaceFailed, "无法替换场景文件", replaceError);



        return true;
    }
}
