//
// Created by chenlong on 2026/8/11.
//
#include "Limen/RHI/Shader.h"

#include "Limen/Core/Log.h"
#include "RHI/macOS/OpenGL/OpenGLShader.h"
#include "Limen/Renderer/Renderer.h"
#include <fstream>
#include <sstream>

namespace Limen
{
    namespace
    {
        /**
         * @brief 判断路径是否指向一个存在的普通文件。
         *
         * @param path
         * 要检查的文件路径。
         *
         * @return
         * 文件存在且是普通文件时返回true，否则返回false。
         *
         * 使用std::error_code避免filesystem在路径错误时抛出异常，
         * 在Clang、GCC和MSVC上行为一致。
         */
        [[nodiscard]]
        bool IsRegularFile(
            const std::filesystem::path &path
        )
        {
            std::error_code error;

            const bool result =
                    std::filesystem::is_regular_file(
                        path,
                        error
                    );

            return !error && result;
        }

        /**
         * @brief 判断路径是否以指定的目录组件开头。
         *
         * @param path 要检查的相对路径。
         * @param directory 期望位于第一个位置的目录名。
         */
        [[nodiscard]]
        bool StartsWithDirectory(
            const std::filesystem::path &path,
            const std::filesystem::path &directory
        )
        {
            const auto firstComponent = path.begin();

            return firstComponent != path.end() &&
                   *firstComponent == directory;
        }

        /**
         * @brief 解析Shader文件路径。
         *
         * 解析顺序：
         *
         * 1. 绝对路径直接使用；
         * 2. 已以assets开头时不重复拼接；
         * 3. 旧的shaders/...写法会兼容为assets/shaders/...；
         * 4. 其他相对路径优先尝试assets/shaders/requestedPath；
         * 5. 拼接结果不存在时，回退到调用者传入的路径。
         *
         * @param requestedPath 调用者传入的Shader文件路径。
         */
        [[nodiscard]]
        std::filesystem::path ResolveShaderPath(
            const std::filesystem::path &requestedPath
        )
        {
            if (requestedPath.empty())
                return {};

            // 消除多余的分隔符以及 . 等路径组件。
            const std::filesystem::path normalizedPath =
                    requestedPath.lexically_normal();

            // 绝对路径由调用方完整指定，不再拼接 assets/shaders。
            if (normalizedPath.is_absolute())
                return normalizedPath;

            std::filesystem::path assetCandidate;

            if (StartsWithDirectory(normalizedPath, "assets"))
            {
                assetCandidate = normalizedPath;
            } else if (StartsWithDirectory(normalizedPath, "shaders"))
            {
                assetCandidate = std::filesystem::path("assets") / normalizedPath;
            } else
            {
                assetCandidate = std::filesystem::path("assets") /
                                 "shaders" /
                                 normalizedPath;
            }

            if (IsRegularFile(assetCandidate))
                return assetCandidate;

            return normalizedPath;
        }

        /**
         * @brief 将一个文本文件完整读取为std::string。
         *
         * @param requestedPath 调用者传入的路径。
         * @return 成功时返回文件内容，失败时返回空字符串。
         */
        [[nodiscard]]
        std::string ReadTextFile(
            const std::filesystem::path &requestedPath
        )
        {
            const std::filesystem::path resolvedPath =
                    ResolveShaderPath(requestedPath);

            std::ifstream input(
                resolvedPath,
                std::ios::in | std::ios::binary
            );

            if (!input.is_open())
            {
                LM_CORE_ERROR(
                    "Failed to open shader file. Requested='{}', Resolved='{}'",
                    requestedPath.string(),
                    resolvedPath.string()
                );

                return {};
            }

            std::ostringstream sourceStream;
            sourceStream << input.rdbuf();

            if (input.bad())
            {
                LM_CORE_ERROR(
                    "Failed while reading shader file '{}'",
                    resolvedPath.string()
                );

                return {};
            }

            return sourceStream.str();
        }

        /**
         * @brief 从路径的文件主名提取Shader名称。
         *
         * 例如BlinnPhong.asdasdsa的stem()为BlinnPhong。
         */
        [[nodiscard]]
        std::string ExtractShaderName(
            const std::filesystem::path &path
        )
        {
            std::filesystem::path stem = path.stem();

            // 支持BlinnPhong.vs.hlsl和BlinnPhong.ps.hlsl这类双扩展名。
            const std::string stageExtension = stem.extension().string();
            if (stageExtension == ".vs" ||
                stageExtension == ".ps" ||
                stageExtension == ".vert" ||
                stageExtension == ".frag")
            {
                stem = stem.stem();
            }

            return stem.string();
        }

        [[nodiscard]]
        bool ContainsParentTraversal(const std::filesystem::path& path)
        {
            for (const auto& component : path)
            {
                if (component == "..")
                    return true;
            }

            return false;
        }

        /**
         * @brief 根据当前图形API生成Vertex/Fragment(Pixel) Shader路径。
         */
        [[nodiscard]]
        bool BuildBackendGraphicsShaderPaths(
            const std::filesystem::path& logicalPath,
            std::filesystem::path& vertexPath,
            std::filesystem::path& fragmentPath
        )
        {
            std::filesystem::path backendDirectory;
            std::string vertexSuffix;
            std::string fragmentSuffix;

            switch (Renderer::GetRenderAPI())
            {
                case RendererAPI::API::OPENGL:
                    backendDirectory = "OpenGL";
                    vertexSuffix = ".vert";
                    fragmentSuffix = ".frag";
                    break;

                case RendererAPI::API::DIRECT11:
                    backendDirectory = "DirectX11";
                    vertexSuffix = ".vs.hlsl";
                    fragmentSuffix = ".ps.hlsl";
                    break;

                case RendererAPI::API::DIRECT12:
                    backendDirectory = "DirectX12";
                    vertexSuffix = ".vs.hlsl";
                    fragmentSuffix = ".ps.hlsl";
                    break;

                case RendererAPI::API::METAL:
                    backendDirectory = "Metal";
                    vertexSuffix = ".vert.metal";
                    fragmentSuffix = ".frag.metal";
                    break;

                case RendererAPI::API::VULKAN:
                    backendDirectory = "Vulkan";
                    vertexSuffix = ".vert.glsl";
                    fragmentSuffix = ".frag.glsl";
                    break;

                case RendererAPI::API::NONE:
                    LM_CORE_ERROR("Cannot resolve Shader files when RendererAPI is NONE");
                    return false;
            }

            const std::filesystem::path backendPath =
                backendDirectory / logicalPath;

            vertexPath = backendPath;
            vertexPath += vertexSuffix;

            fragmentPath = backendPath;
            fragmentPath += fragmentSuffix;

            return true;
        }
    }

    Ref<Shader> Shader::CreateFromSource(
        const std::string &name,
        const std::string &vertexSource,
        const std::string &fragmentSource)
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::OPENGL:
                return CreateRef<OpenGLShader>(name, vertexSource, fragmentSource);

            case RendererAPI::API::DIRECT12:
                LM_CORE_ERROR("DirectX 12 Shader creation is not implemented");
                return nullptr;

            case RendererAPI::API::NONE:
                LM_CORE_ERROR("Cannot create a Shader when RenderAPI is NONE");
                return nullptr;

            default:
                LM_CORE_ERROR("The selected RenderAPI does not implement Shader creation yet");
                return nullptr;
        }
    }

    Ref<Shader> Shader::Create(
        const std::string &name,
        const std::string &vertexSource,
        const std::string &fragmentSource)
    {
        return CreateFromSource(
            name,
            vertexSource,
            fragmentSource
        );
    }

    Ref<Shader> Shader::CreateFromFiles(
        const std::filesystem::path &vertexPath,
        const std::filesystem::path &fragmentPath
    )
    {
        const std::string shaderName =
                ExtractShaderName(vertexPath);

        if (shaderName.empty())
        {
            LM_CORE_ERROR(
                "Cannot extract Shader name from vertex path '{}'",
                vertexPath.string()
            );

            return nullptr;
        }

        return CreateFromFiles(
            shaderName,
            vertexPath,
            fragmentPath
        );
    }

    Ref<Shader> Shader::CreateFromFiles(
        std::string name,
        const std::filesystem::path &vertexPath,
        const std::filesystem::path &fragmentPath
    )
    {
        if (name.empty())
            name = ExtractShaderName(vertexPath);

        if (name.empty())
        {
            LM_CORE_ERROR(
                "Shader name is empty and cannot be extracted from '{}'",
                vertexPath.string()
            );

            return nullptr;
        }

        const std::string vertexFileName =
                ExtractShaderName(vertexPath);

        const std::string fragmentFileName =
                ExtractShaderName(fragmentPath);

        if (!vertexFileName.empty() &&
            !fragmentFileName.empty() &&
            vertexFileName != fragmentFileName)
        {
            LM_CORE_WARN(
                "Vertex Shader '{}' and Fragment Shader '{}' use different file names; logical name is '{}'",
                vertexFileName,
                fragmentFileName,
                name
            );
        }

        const std::string vertexSource =
                ReadTextFile(vertexPath);

        const std::string fragmentSource =
                ReadTextFile(fragmentPath);

        if (vertexSource.empty())
        {
            LM_CORE_ERROR(
                "Vertex shader source is empty: '{}'",
                vertexPath.string()
            );

            return nullptr;
        }

        if (fragmentSource.empty())
        {
            LM_CORE_ERROR(
                "Fragment shader source is empty: '{}'",
                fragmentPath.string()
            );

            return nullptr;
        }
        /**
         * 复用现有的源码创建流程。
         *
         * OpenGL后端仍然负责：
         *
         * glCreateShader
         * glCompileShader
         * glCreateProgram
         * glLinkProgram
         */
        return CreateFromSource(
            name,
            vertexSource,
            fragmentSource
        );
    }

    void ShaderLibrary::Add(const Ref<Shader> &shader)
    {
        if (!shader)
        {
            LM_CORE_ERROR("Cannot add a null Shader to ShaderLibrary");
            return;
        }

        const std::string &name = shader->GetName();

        if (name.empty())
        {
            LM_CORE_ERROR("Cannot add an unnamed Shader to ShaderLibrary");
            return;
        }

        const bool inserted =
                m_Shaders.emplace(name, shader).second;

        if (!inserted)
        {
            LM_CORE_WARN(
                "Shader '{}' already exists in ShaderLibrary; keeping the existing Shader",
                name
            );
        }
    }

    Ref<Shader> ShaderLibrary::Load(
        const std::filesystem::path& logicalPath
    )
    {
        const std::filesystem::path normalizedPath =
            logicalPath.lexically_normal();

        if (normalizedPath.empty() ||
            normalizedPath.is_absolute() ||
            ContainsParentTraversal(normalizedPath))
        {
            LM_CORE_ERROR(
                "Shader logical path must be a safe relative path: '{}'",
                logicalPath.string()
            );
            return nullptr;
        }

        if (normalizedPath.has_extension())
        {
            LM_CORE_ERROR(
                "Shader logical path must not contain a stage extension: '{}'. Use a path such as 'Example3D/BlinnPhong'",
                logicalPath.string()
            );
            return nullptr;
        }

        const std::string shaderName =
            normalizedPath.generic_string();

        if (Exists(shaderName))
            return Get(shaderName);

        std::filesystem::path vertexPath;
        std::filesystem::path fragmentPath;
        if (!BuildBackendGraphicsShaderPaths(
                normalizedPath,
                vertexPath,
                fragmentPath))
        {
            return nullptr;
        }

        return Load(
            shaderName,
            vertexPath,
            fragmentPath
        );
    }

    Ref<Shader> ShaderLibrary::Load(
        const std::filesystem::path &vertexPath,
        const std::filesystem::path &fragmentPath
    )
    {
        return Load(
            ExtractShaderName(vertexPath),
            vertexPath,
            fragmentPath
        );
    }

    Ref<Shader> ShaderLibrary::Load(
        std::string name,
        const std::filesystem::path &vertexPath,
        const std::filesystem::path &fragmentPath
    )
    {
        if (name.empty())
            name = ExtractShaderName(vertexPath);

        if (name.empty())
        {
            LM_CORE_ERROR(
                "Cannot load Shader with an empty name from '{}'",
                vertexPath.string()
            );

            return nullptr;
        }

        if (Exists(name))
        {
            LM_CORE_WARN(
                "Shader '{}' is already loaded; reusing the cached Shader",
                name
            );

            return Get(name);
        }

        Ref<Shader> shader = Shader::CreateFromFiles(
            name,
            vertexPath,
            fragmentPath
        );

        if (shader)
            Add(shader);

        return shader;
    }

    Ref<Shader> ShaderLibrary::Get(
        const std::string &name
    ) const
    {
        const auto shader = m_Shaders.find(name);

        if (shader == m_Shaders.end())
        {
            LM_CORE_ERROR(
                "Shader '{}' was not found in ShaderLibrary",
                name
            );

            return nullptr;
        }

        return shader->second;
    }

    bool ShaderLibrary::Exists(
        const std::string &name
    ) const noexcept
    {
        return m_Shaders.contains(name);
    }

}
