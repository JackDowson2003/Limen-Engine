//
// Created by chenlong on 2026/8/28.
//
#include <type_traits>
#include "Limen/Renderer/Material.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Texture.h"

namespace Limen
{
    Material::Material(Ref<GraphicsPipeline> pipeline, std::string debugName)
        : m_Pipeline(std::move(pipeline)), m_DebugName(std::move(debugName))
    {
        // Material必须拥有一条有效Pipeline，
        // 后续Bind()需要通过它绑定Shader和固定渲染状态。
        LM_CORE_ASSERT(
            m_Pipeline,
            "Material '{}' requires a valid GraphicsPipeline",
            m_DebugName
        );
    }

    void Material::SetFloat(const std::string &name, const float value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetFloat received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetInt(const std::string &name, const int32_t value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetInt received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetUInt(const std::string &name, const uint32_t value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetUInt received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetFloat2(const std::string &name, const glm::vec2 &value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetFloat2 received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetFloat3(const std::string &name, const glm::vec3 &value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetFloat3 received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetFloat4(const std::string &name, const glm::vec4 &value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetFloat4 received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetMat4(const std::string &name, const glm::mat4 &value)
    {
        LM_CORE_ASSERT(
            !name.empty(),
            "Material::SetMat4 received an empty parameter name"
        );
        if (name.empty())
            return;

        // 不存在时插入，已经存在时覆盖旧值。
        m_Parameters.insert_or_assign(
            name,
            value
        );
    }

    void Material::SetTexture(const std::string &samplerName, const Ref<Texture> &texture, const uint32_t slot)
    {
        LM_CORE_ASSERT(
            texture,
            "Material texture cannot be null"
        );
        LM_CORE_ASSERT(
            !samplerName.empty(),
            "Material::SetTexture received an empty sampler name"
        );
        if (samplerName.empty() || !texture)
            return;

        m_TexturesBindings.insert_or_assign(
            samplerName,
            MaterialTextureBinding{
                .TextureRes = texture,
                .Slot = slot
            }
        );
    }

    void Material::Bind() const
    {
        LM_CORE_ASSERT(
            m_Pipeline,
            "Material '{}' has no GraphicsPipeline",
            m_DebugName
        );

        if (!m_Pipeline)
            return;

        // Pipeline::Bind()会绑定Shader并应用深度、混合、剔除等状态。
        // Uniform必须在对应Shader绑定之后上传。
        m_Pipeline->Bind();

        const Ref<Shader> &shader = m_Pipeline->GetSpecification().ShaderProgram;

        LM_CORE_ASSERT(
            shader,
            "Material '{}' pipeline has no Shader",
            m_DebugName
        );

        if (!shader)
            return;

        //遍历所有已经保存的Shader参数 并设置到Shader
        for (const auto &[name,parameter]: m_Parameters)
        {
            /**
             * parameter是std::variant。
             *
             * std::visit会访问variant当前保存的实际类型，
             * 例如float、glm::vec3或glm::mat4。
             */
            std::visit(
                [&shader,&name]<typename T>(const T &val)
                {
                    /**
                     * decltype(val)得到的类型可能是const float&。
                     *
                     * decay_t会去掉const和引用，得到原始类型float。
                     */
                    using ValueType = std::decay_t<T>;

                    if constexpr (std::is_same_v<ValueType, float>)
                    {
                        shader->SetFloat(name.c_str(), val);
                    } else if constexpr (std::is_same_v<ValueType, int32_t>)
                    {
                        shader->SetInt(name.c_str(), val);
                    } else if constexpr (std::is_same_v<ValueType, uint32_t>)
                    {
                        shader->SetInt(name.c_str(), val);
                    } else if constexpr (std::is_same_v<ValueType, glm::vec2>)
                    {
                        shader->SetFloat2(name.c_str(), val);
                    } else if constexpr (std::is_same_v<ValueType, glm::vec3>)
                    {
                        shader->SetFloat3(name.c_str(), val);
                    } else if constexpr (std::is_same_v<ValueType, glm::vec4>)
                    {
                        shader->SetFloat4(name.c_str(), val);
                    } else if constexpr (std::is_same_v<ValueType, glm::mat4>)
                    {
                        shader->SetMat4(name.c_str(), val);
                    }
                },
                parameter
            );
        }

        for (const auto &[samplerName, texture]: m_TexturesBindings)
        {
            LM_CORE_ASSERT(
                texture.TextureRes,
                "Material '{}' sampler '{}' has no Texture",
                m_DebugName,
                samplerName
            );
            if (!texture.TextureRes)
                continue;
            //把真实纹理绑定到slot
            texture.TextureRes->Bind(texture.Slot);

            // sampler保存的不是纹理ID，而是纹理槽编号
            shader->SetInt(samplerName.c_str(), texture.Slot);
        }
    }

    const GraphicsPipeline &Material::GetPipeline() const
    {
        LM_CORE_ASSERT(m_Pipeline, "Material '{}' has no GraphicsPipeline",
                       m_DebugName
        );
        return *m_Pipeline.get();
    }

    const std::string &Material::GetDebugName() const noexcept
    {
        return m_DebugName;
    }
}
