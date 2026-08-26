//
// Created by Codex on 2026/8/11.
//
#pragma once

#include <string>

#include "Limen/RHI/Shader.h"
#include <glm/glm.hpp>

namespace Limen
{
    class OpenGLShader : public Shader
    {
    public :
        OpenGLShader(const std::string &name, const std::string &vertexSource, const std::string &fragmentSource);

        ~OpenGLShader() override;

        void Bind() const override;

        void UnBind() const override;

        void SetMat4(const char *name, const glm::mat4 &matrix) override;

        void SetFloat3(const char *name, const glm::vec3 &value) override;

        void SetInt(
            const char *name,
            int value
        ) override;

        void SetInt(
            const char* name,
            uint32_t value
        ) override;

        void SetUniformBufferBinding(
            const char *blockName,
            uint32_t binding
        ) override;

        [[nodiscard]]
        const std::string &GetName() const noexcept override
        {
            return m_Name;
        }

        void UploadUniformMat3(const char *name, const glm::mat3 &val);

        void UploadUniformMat4(const char *name, const glm::mat4 &val);

        void UploadUniformInt(const char *name, int value);

        void UploadUniformFloat(const char *name, float value);

        void UploadUniformFloat2(const char *name, const glm::vec2 &val);

        void UploadUniformFloat3(const char *name, const glm::vec3 &val);

        void UploadUniformFloat4(const char *name, const glm::vec4 &val);

        int GetUniformLocation(const char *name);

    private:
        uint32_t m_RendererID = 0;
        std::unordered_map<std::string, int> m_UniformLocations;
        std::string m_Name;
    };
}
