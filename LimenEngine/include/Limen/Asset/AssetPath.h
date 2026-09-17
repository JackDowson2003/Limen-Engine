//
// Created by chenlong on 2026/9/16.
//

#pragma once

#include <filesystem>

#include "Limen/Core/Core.h"

namespace Limen
{
    /**
     * @brief 统一管理引擎资源根目录和逻辑资源路径解析
     *
     * 客户端只提供相对资源根目录的逻辑路径，例如
     *
     * models/TestTwoMaterials.obj
     * models/TestTwoMaterials.obj
     *
     * AssetPath 负责将它转换为当前运行环境中的实际路径
     */
    class LIMEN_API AssetPath final
    {
    public:
        // This class is the static res root directories of tools
        // Cannot create instance
        AssetPath() = delete;

        /**
         * @brief 设置当前程序使用的资源根目录。
         *
         * 应在加载任何Shader、Texture或Model之前调用。
         *
         * @param root
         * 可以是相对路径，例如"assets"；
         * 也可以是绝对路径。
         */
        static void SetRoot(const std::filesystem::path &root);

        /**
         * @brief 获取当前资源根目录。
         */
        [[nodiscard]]
        static const std::filesystem::path &GetRoot() noexcept;

        /**
         * @brief 将逻辑资源路径解析为实际文件路径。
         *
         * 相对路径会拼接到资源根目录之后；
         * 绝对路径不会再次拼接资源根目录。
         *
         * @param logicalPath
         * 例如"models/TestTwoMaterials.obj"。
         *
         * @return
         * 经过词法规范化的实际资源路径。
         */
        [[nodiscard]]
        static std::filesystem::path Resolve(const std::filesystem::path &logicalPath);

    private:
        // 默认对应当前可执行文件旁边的assets目录
        static std::filesystem::path s_Root;
    };
}
