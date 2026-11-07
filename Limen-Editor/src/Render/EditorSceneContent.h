//
// Created by chenlong on 2026/11/7.
//

#pragma once

#include "Limen/Core/Core.h"
#include "Limen/RHI/Shader.h"

namespace Limen
{
    class Mesh;
    class Material;
}

namespace LimenEditor
{
    // Editor 默认可渲染内容；GPU 资源由渲染层持有，Scene 只借用 Ref
    class EditorSceneContent final
    {
    public:
        [[nodiscard]]
        bool Initialize();

        [[nodiscard]]
        const Limen::Ref<Limen::Mesh>& GetCubeMesh() const noexcept
        {
            return m_CubeMesh;
        }

        [[nodiscard]] const Limen::Ref<Limen::Material> &GetCubeMaterial() const noexcept
        {
            return m_CubeMaterial;
        }

    private:
        Limen::Ref<Limen::ShaderLibrary> m_ShaderLibrary = Limen::CreateRef<Limen::ShaderLibrary>();
        Limen::Ref<Limen::Mesh> m_CubeMesh;
        Limen::Ref<Limen::Material> m_CubeMaterial;
    };
}
