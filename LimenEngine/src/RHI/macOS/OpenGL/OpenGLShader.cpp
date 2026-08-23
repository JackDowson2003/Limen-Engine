//
// Created by Codex on 2026/8/11.
//
#include "OpenGLShader.h"

#include <glad/gl.h>

#include <string>
#include <vector>

#include "Limen/Core/Log.h"
#include "glm/gtc/type_ptr.hpp"

namespace Limen
{
    namespace
    {
        [[nodiscard]] const char *ShaderStageName(const GLenum stage)
        {
            switch (stage)
            {
                case GL_VERTEX_SHADER: return "Vertex";
                case GL_FRAGMENT_SHADER: return "Fragment";
                default: return "Unknown";
            }
        }

        [[nodiscard]] uint32_t CompileShader(const GLenum stage, const std::string &source)
        {
            const uint32_t shader = glCreateShader(stage);
            const char *sourceData = source.c_str();
            const auto sourceLength = static_cast<GLint>(source.size());

            glShaderSource(shader, 1, &sourceData, &sourceLength);
            glCompileShader(shader);

            GLint compiled = GL_FALSE;
            glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);
            if (compiled == GL_TRUE)
                return shader;

            GLint logLength = 0;
            glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);

            std::vector<GLchar> infoLog(static_cast<size_t>(logLength > 0 ? logLength : 1));
            GLsizei written = 0;
            glGetShaderInfoLog(shader, logLength, &written, infoLog.data());

            LM_CORE_ERROR("{} shader compilation failed:\n{}",
                          ShaderStageName(stage),
                          std::string(infoLog.data(), static_cast<size_t>(written)));

            glDeleteShader(shader);
            return 0;
        }
    }

    OpenGLShader::OpenGLShader(
        const std::string &name,
        const std::string &vertexSource,
        const std::string &fragmentSource)
            :m_Name(name)
    {
        const uint32_t vertexShader = CompileShader(GL_VERTEX_SHADER, vertexSource);
        const uint32_t fragmentShader = CompileShader(GL_FRAGMENT_SHADER, fragmentSource);

        if (vertexShader == 0 || fragmentShader == 0)
        {
            if (vertexShader != 0)
                glDeleteShader(vertexShader);
            if (fragmentShader != 0)
                glDeleteShader(fragmentShader);

            LM_CORE_ASSERT(false, "OpenGL shader compilation failed");
            return;
        }

        m_RendererID = glCreateProgram();
        glAttachShader(m_RendererID, vertexShader);
        glAttachShader(m_RendererID, fragmentShader);
        glLinkProgram(m_RendererID);

        GLint linked = GL_FALSE;
        glGetProgramiv(m_RendererID, GL_LINK_STATUS, &linked);

        if (linked != GL_TRUE)
        {
            GLint logLength = 0;
            glGetProgramiv(m_RendererID, GL_INFO_LOG_LENGTH, &logLength);

            std::vector<GLchar> infoLog(static_cast<size_t>(logLength > 0 ? logLength : 1));
            GLsizei written = 0;
            glGetProgramInfoLog(m_RendererID, logLength, &written, infoLog.data());

            LM_CORE_ERROR("OpenGL shader program linking failed:\n{}",
                          std::string(infoLog.data(), static_cast<size_t>(written)));

            glDeleteProgram(m_RendererID);
            m_RendererID = 0;
        } else
        {
            // Program 链接完成后已经保留了可执行代码，独立 Shader 对象可以释放。
            glDetachShader(m_RendererID, vertexShader);
            glDetachShader(m_RendererID, fragmentShader);
        }

        glDeleteShader(vertexShader);
        glDeleteShader(fragmentShader);

        LM_CORE_ASSERT(m_RendererID != 0, "OpenGL shader program linking failed");
    }

    OpenGLShader::~OpenGLShader()
    {
        if (m_RendererID != 0)
            glDeleteProgram(m_RendererID);
    }

    void OpenGLShader::Bind() const
    {
        LM_CORE_ASSERT(m_RendererID != 0, "Cannot bind an invalid OpenGL shader");
        glUseProgram(m_RendererID);
    }

    void OpenGLShader::UnBind() const
    {
        glUseProgram(0);
    }

    void OpenGLShader::SetMat4(const char *name, const glm::mat4 &matrix)
    {
        if (name ==nullptr)
        {
            LM_CORE_ERROR("name is null");
            return;
        }
        UploadUniformMat4(name,matrix);
    }

    void OpenGLShader::SetFloat3(const char *name, const glm::vec3& value)
    {
        if (name ==nullptr)
        {
            LM_CORE_ERROR("name is null");
            return;
        }
        UploadUniformFloat3(name,value);
    }

    void OpenGLShader::SetInt(const char *name, int value)
    {
        if (name ==nullptr)
        {
            LM_CORE_ERROR("name is null");
            return;
        }
        UploadUniformInt(name, value);
    }

    void OpenGLShader::SetUniformBufferBinding(const char *blockName, uint32_t binding)
    {
        if (blockName == nullptr)
        {
            LM_CORE_ERROR("Uniform buffer block name is null");
            return;
        }

        const GLuint blockIndex = glGetUniformBlockIndex(
            m_RendererID,
            blockName
        );

        LM_CORE_ASSERT(
            blockIndex != GL_INVALID_INDEX,
            "Uniform block '{}' was not found",
            blockName
        );

        if (blockIndex == GL_INVALID_INDEX)
            return;

        glUniformBlockBinding(
            m_RendererID,
            blockIndex,
            binding
        );
    }

    void OpenGLShader::UploadUniformMat4(const char* name,const glm::mat4& val)
    {
        const GLint location = GetUniformLocation(name);
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(val));
    }



    void OpenGLShader::UploadUniformMat3(const char *name, const glm::mat3 &val)
    {
        const GLint location = GetUniformLocation(name);
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(val));
    }

    void OpenGLShader::UploadUniformInt(const char *name, int value)
    {
        const GLint location = GetUniformLocation(name);
        glUniform1i(location, value);
    }

    void OpenGLShader::UploadUniformFloat(const char *name, const float value)
    {
        const GLint location = GetUniformLocation(name);
        glUniform1f(location, value);
    }

    void OpenGLShader::UploadUniformFloat2(const char *name, const glm::vec2 &val)
    {
        const GLint location = GetUniformLocation(name);
        glUniform2f(location, val.x, val.y);
    }

    void OpenGLShader::UploadUniformFloat3(const char *name, const glm::vec3 &val)
    {
        const GLint location = GetUniformLocation(name);
        glUniform3f(location, val.x, val.y, val.z);
    }

    void OpenGLShader::UploadUniformFloat4(const char *name, const glm::vec4& val)
    {
        const GLint location = GetUniformLocation(name);
        glUniform4f(location, val.x, val.y, val.z, val.w);
    }



    int OpenGLShader::GetUniformLocation(const char *name)
    {
        if (m_UniformLocations.contains(name))
            return m_UniformLocations[name];

        const GLint & location = glGetUniformLocation(m_RendererID, name);
        m_UniformLocations[name] = location;
        return location;
    }

}
