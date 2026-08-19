//
// Created by Codex on 2026/8/11.
//
#pragma once

#include <cstdint>
#include <string>

#include "glm/detail/type_vec3.hpp"
#include "Renderer/Shader.h"

namespace Limen
{
    class OpenGLShader final : public Shader
    {
    public :
        OpenGLShader(const std::string &vertexSource, const std::string &fragmentSource);

        ~OpenGLShader() override;

        void Bind() const override;

        void UnBind() const override;

        void UploadUniformMat3(const char *name, const glm::mat4 &val);
        void UploadUniformMat4(const char *name, const glm::mat4 &val);

        void UploadUniformInt(const char *name,  int value);

        void UploadUniformFloat(const char *name,  float value);
        void UploadUniformFloat2(const char *name, const glm::vec2 &val);
        void UploadUniformFloat3(const char *name, const glm::vec3 &val);
        void UploadUniformFloat4(const char *name, const glm::vec4 &val);

        int GetUniformLocation(const char *name);


    private:
        uint32_t m_RendererID = 0;
        std::unordered_map<const char *, int> m_UniformLocations;
    };
}
