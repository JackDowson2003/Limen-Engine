//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include <filesystem>
#include <string>

#include "Core.h"

namespace Limen
{
    class LIMEN_API Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        /**
         * @brief 从内存中的源码字符串创建Shader。
         *
         * @param vertexSource Vertex Shader的完整源码。
         * @param fragmentSource Fragment Shader的完整源码。
         *
         * @return 创建成功时返回Shader共享引用，否则返回nullptr。
         */
        [[nodiscard]]
        static Ref<Shader> CreateFromSource(
            const std::string &vertexSource,
            const std::string &fragmentSource
        );

        /**
         * @brief 从两个Shader源码文件创建一个图形Shader Program。
         *
         * 该函数负责：
         *
         * 1. 读取Vertex Shader文件；
         * 2. 读取Fragment Shader文件；
         * 3. 把读取到的源码交给当前Renderer后端编译和链接。
         *
         * @param vertexPath
         * Vertex Shader源码文件路径。
         * OpenGL阶段通常使用.vert后缀。
         *
         * @param fragmentPath
         * Fragment Shader源码文件路径。
         * OpenGL阶段通常使用.frag后缀。
         *
         * @return
         * 创建成功时返回Shader的共享引用；
         * 文件读取或Shader创建失败时返回nullptr。
         */
        [[nodiscard]]
        static Ref<Shader> CreateFromFiles(
            const std::filesystem::path& vertexPath,
            const std::filesystem::path& fragmentPath
        );

        /**
         * @brief 旧的源码创建入口，仅用于过渡期兼容。
         *
         * @deprecated
         * 请根据输入类型改用CreateFromSource()或CreateFromFiles()。
         *
         * [[deprecated]]是标准Cpp 14以上支持的，Clang、GCC和MSVC都支持。
         */
        [[nodiscard]]
        [[deprecated(
            "This function is deprecated, use Shader::CreateFromSource() or Shader::CreateFromFiles() instead"
        )]]
        static Ref<Shader> Create(
            const std::string &vertexSource,
            const std::string &fragmentSource
        );
    };

}
