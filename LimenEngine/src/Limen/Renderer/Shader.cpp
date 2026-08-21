//
// Created by chenlong on 2026/8/11.
//
#include "Renderer/Shader.h"

#include "Log.h"
#include "Platform/Mac/OpenGL/OpenGLShader.h"
#include "Renderer/Renderer.h"
#include <fstream>
#include <sstream>

namespace Limen
{
    namespace
    {
        /**
         * @brief 将一个文本文件完整读取为std::string。
         *
         * @param path
         * 要读取的文件路径。
         *
         * @return
         * 读取成功时返回完整文件内容；
         * 打开或读取失败时返回空字符串。
         */
        [[nodiscard]]
        std::string ReadTextFile(
            const std::filesystem::path& path
        )
        {
            std::ifstream input(
                path,
                std::ios::in | std::ios::binary
            );

            if (!input.is_open())
            {
                LM_CORE_ERROR("Failed to open shader file '{}'",path.string());

                return {};
            }

            std::ostringstream sourceStream;
            sourceStream << input.rdbuf(); //Shader文件足够了 而且底层会分块写入的

            if (input.bad())
            {
                LM_CORE_ERROR(
                    "Failed while reading shader file '{}'",
                    path.string()
                );

                return {};
            }

            //无需缓存 OS有文件页的缓存 而且我们一般会复用shader 或者move 不回来重新读
            return sourceStream.str();
        }
    }

    Ref<Shader> Shader::Create(
        const std::string &vertexSource,
        const std::string &fragmentSource)
    {
        switch (Renderer::GetRenderAPI())
        {
            case RendererAPI::API::OPENGL:
                return CreateRef<OpenGLShader>(vertexSource, fragmentSource);

            case RendererAPI::API::DIRECT12:
                LM_CORE_ERROR("Cannot create a Shader when RenderAPI is NONE");
                return nullptr;

            case RendererAPI::API::NONE:
                LM_CORE_ERROR("Cannot create a Shader when RenderAPI is NONE");
                return nullptr;

            default:
                LM_CORE_ERROR("The selected RenderAPI does not implement Shader creation yet");
                return nullptr;
        }
    }

    Ref<Shader> Shader::CreateFromFiles(
    const std::filesystem::path& vertexPath,
    const std::filesystem::path& fragmentPath
)
    {
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
        return Create(
            vertexSource,
            fragmentSource
        );
    }

}
