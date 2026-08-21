//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include "Core.h"

namespace Limen
{
    class LIMEN_API Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;
        virtual void UnBind() const = 0;

        // 根据当前 Renderer API 创建对应的 Shader 实现。
        [[nodiscard]] static Ref<Shader> Create(
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
    };

}
