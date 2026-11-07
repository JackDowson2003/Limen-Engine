#include "Render/EditorSceneContent.h"

#include "Limen/Asset/AssetManager.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Texture.h"
#include "Limen/Renderer/Material.h"
#include "Limen/Renderer/Mesh.h"

namespace LimenEditor
{
    namespace
    {
        void AddFace(
            Limen::MeshData &data,
            const glm::vec3 &normal,
            const glm::vec3 &tangent,
            const glm::vec3 &p0,
            const glm::vec3 &p1,
            const glm::vec3 &p2,
            const glm::vec3 &p3)
        {
            const uint32_t base = data.Vertices.getSize();
            const glm::vec3 positions[4]{p0, p1, p2, p3};
            const glm::vec2 texCoords[4]{
                {0.0f, 0.0f}, {1.0f, 0.0f}, {1.0f, 1.0f}, {0.0f, 1.0f}
            };

            for (uint32_t i = 0; i < 4; ++i)
            {
                data.Vertices.push_back(Limen::MeshVertex{
                    .Position = positions[i],
                    .Normal = normal,
                    .TexCoord = texCoords[i],
                    .Tangent = glm::vec4{tangent, 1.0f}
                });
            }

            const uint32_t offsets[6]{0, 1, 2, 2, 3, 0};
            for (const uint32_t offset: offsets)
                data.Indices.emplace_back(base + offset);
        }
    }

    bool EditorSceneContent::Initialize()
    {
        Limen::MeshData cubeData;
        AddFace(cubeData, {0, 0, 1}, {1, 0, 0},
                {-1, -1, 1}, {1, -1, 1}, {1, 1, 1}, {-1, 1, 1});
        AddFace(cubeData, {0, 0, -1}, {-1, 0, 0},
                {1, -1, -1}, {-1, -1, -1}, {-1, 1, -1}, {1, 1, -1});
        AddFace(cubeData, {-1, 0, 0}, {0, 0, 1},
                {-1, -1, -1}, {-1, -1, 1}, {-1, 1, 1}, {-1, 1, -1});
        AddFace(cubeData, {1, 0, 0}, {0, 0, -1},
                {1, -1, 1}, {1, -1, -1}, {1, 1, -1}, {1, 1, 1});
        AddFace(cubeData, {0, 1, 0}, {1, 0, 0},
                {-1, 1, 1}, {1, 1, 1}, {1, 1, -1}, {-1, 1, -1});
        AddFace(cubeData, {0, -1, 0}, {1, 0, 0},
                {-1, -1, -1}, {1, -1, -1}, {1, -1, 1}, {-1, -1, 1});

        m_CubeMesh = Limen::CreateRef<Limen::Mesh>(cubeData);
        if (!m_CubeMesh)
        {
            LM_ERROR("Editor scene content: failed to create cube mesh");
            return false;
        }

        const Limen::Ref<Limen::Shader> shader =
            m_ShaderLibrary->Load("Example3D/BlinnPhong");
        if (!shader)
        {
            LM_ERROR("Editor scene content: failed to load BlinnPhong shader");
            return false;
        }

        Limen::GraphicsPipelineSpecification pipelineSpec;
        pipelineSpec.ShaderProgram = shader;
        pipelineSpec.Topology = Limen::PrimitiveTopology::TriangleList;
        pipelineSpec.DepthTestEnabled = true;
        pipelineSpec.DepthWriteEnabled = true;
        pipelineSpec.DepthCompare = Limen::CompareOperation::Less;
        pipelineSpec.Blend = Limen::BlendMode::Opaque;
        pipelineSpec.Culling = Limen::CullMode::None;
        pipelineSpec.FrontFaceWinding = Limen::FrontFace::CounterClockwise;
        pipelineSpec.DebugName = "Editor Default Cube Pipeline";

        const Limen::Ref<Limen::GraphicsPipeline> pipeline =
            Limen::GraphicsPipeline::Create(pipelineSpec);
        if (!pipeline)
        {
            LM_ERROR("Editor scene content: failed to create cube pipeline");
            return false;
        }

        m_CubeMaterial = Limen::CreateRef<Limen::Material>(
            pipeline, "Editor Default Cube Material");
        if (!m_CubeMaterial)
        {
            LM_ERROR("Editor scene content: failed to create cube material");
            return false;
        }

        const Limen::Ref<Limen::Texture2D> &white =
            Limen::AssetManager::GetWhiteTexture();
        const Limen::Ref<Limen::Texture2D> &flatNormal =
            Limen::AssetManager::GetFlatNormalTexture();
        if (!white || !flatNormal)
        {
            LM_ERROR("Editor scene content: default textures unavailable");
            return false;
        }

        m_CubeMaterial->SetTexture("u_AlbedoTexture", white, 0);
        m_CubeMaterial->SetTexture("u_NormalTexture", flatNormal, 1);
        m_CubeMaterial->SetFloat3("u_AmbientReflectance", glm::vec3{0.15f});
        m_CubeMaterial->SetFloat("u_Shininess", 128.0f);
        m_CubeMaterial->SetFloat3(
            "u_DiffuseReflectance", glm::vec3{0.72f, 0.76f, 0.85f});
        m_CubeMaterial->SetFloat3("u_SpecularColor", glm::vec3{0.35f});
        return true;
    }
}