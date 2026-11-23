#include "Render/EditorAssetSpawner.h"

#include <cctype>
#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "Limen/Asset/AssetManager.h"
#include "Limen/Asset/Model.h"
#include "Limen/Asset/ModelMaterialBuilder.h"
#include "Limen/Core/Log.h"
#include "Project/EditorSession.h"
#include "Render/EditorSceneContent.h"

namespace LimenEditor
{
    namespace
    {
        std::string ToLowerExtension(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            for (char &character: extension)
            {
                character = static_cast<char>(std::tolower(
                    static_cast<unsigned char>(character)));
            }
            return extension;
        }

        void SetError(std::string *outError, const std::string_view message)
        {
            if (outError)
                *outError = std::string(message);
        }

        bool IsLabelInUse(
            const EditorSession &session,
            const std::string_view label)
        {
            bool used = false;
            session.GetScene().ForEachObject(
                [&](const Limen::SceneObjectHandle &,
                    const Limen::SceneObjectID &id)
                {
                    if (used)
                        return;

                    Limen::LString existing;
                    if (session.TryGetObjectLabel(id, existing) &&
                        static_cast<std::string_view>(existing) == label)
                        used = true;
                });
            return used;
        }

        std::string MakeUniqueLabel(
            const EditorSession &session,
            const std::string_view base)
        {
            const std::string safeBase =
                base.empty() ? std::string{"Object"} : std::string{base};
            if (!IsLabelInUse(session, safeBase))
                return safeBase;

            for (uint32_t suffix = 2;; ++suffix)
            {
                std::string candidate =
                    safeBase + "_" + std::to_string(suffix);
                if (!IsLabelInUse(session, candidate))
                    return candidate;
            }
        }
    }

    bool EditorAssetSpawner::SpawnSelectedAsset(
        EditorSession &session,
        const EditorSceneContent &sceneContent,
        const Limen::SceneTransform &objectTransform,
        Limen::LArray<Limen::SceneObjectID> &outIDs,
        std::string *outError) const
    {
        outIDs.clear();
        if (outError)
            outError->clear();

        const Limen::AssetID assetID = session.GetSelectedAssetID();
        if (!assetID.isValid())
        {
            SetError(outError, "No asset is selected.");
            return false;
        }

        Limen::AssetRecord record;
        if (!session.GetAssetRegistry().TryFind(assetID, record))
        {
            SetError(outError, "Selected asset is not registered.");
            return false;
        }

        const std::string logicalPath(
            record.LogicalPath.data(),
            record.LogicalPath.size());

        if (record.Status != Limen::AssetRecordStatus::Present)
        {
            LM_WARN("Editor spawn: asset is missing: {}", logicalPath);
            SetError(outError, "Selected asset file is missing.");
            return false;
        }

        // 第一版只支持 OBJ 模型；纹理等资产等待后续资产类型系统。
        if (ToLowerExtension(std::filesystem::path(logicalPath)) != ".obj")
        {
            LM_WARN("Editor spawn: unsupported asset type: {}", logicalPath);
            SetError(outError, "Only OBJ model assets can be placed.");
            return false;
        }

        const Limen::Ref<Limen::Model> model =
            Limen::AssetManager::LoadModel(logicalPath);
        if (!model)
        {
            LM_WARN("Editor spawn: failed to load model: {}", logicalPath);
            SetError(outError, "Failed to load the selected model.");
            return false;
        }

        const Limen::Ref<Limen::GraphicsPipeline> &pipeline =
            sceneContent.GetLitPipeline();
        if (!pipeline)
        {
            SetError(outError, "Editor render pipeline is unavailable.");
            return false;
        }

        const Limen::LArray<Limen::Ref<Limen::Material>> materials =
            Limen::ModelMaterialBuilder::BuildBlinnPhong(*model, pipeline);

        Limen::LArray<Limen::SceneRenderObject> components;
        for (const Limen::ModelPart &part: model->GetParts())
        {
            if (!part.MeshResource)
                continue;

            Limen::Ref<Limen::Material> material;
            if (part.MaterialSlot != Limen::ModelPart::InvalidMaterialSlot &&
                part.MaterialSlot < materials.getSize())
            {
                material = materials[part.MaterialSlot];
            }
            else
            {
                // 没有源材质时使用编辑器默认材质，保证几何仍可见。
                material = sceneContent.GetCubeMaterial();
            }

            if (!material)
                continue;

            components.push_back(Limen::SceneRenderObject{
                .MeshResource = part.MeshResource,
                .MaterialResource = material,
                .Transform = part.LocalTransform
            });
        }

        if (components.getSize() == 0)
        {
            SetError(outError, "Model contains no renderable parts.");
            return false;
        }

        // 一个模型实例 = 一个 SceneObject + 多个渲染组件。
        const std::string label = MakeUniqueLabel(
            session,
            std::filesystem::path(logicalPath).stem().string());
        const Limen::SceneObjectID id =
            session.CreateRenderableObjectGroup(
                components,
                objectTransform,
                label);
        if (!id.isValid())
        {
            SetError(outError, "Failed to create a scene object.");
            return false;
        }

        Limen::SceneObjectComponent modelComponent;
        modelComponent.Data = Limen::ModelRendererComponent{
            .ModelAssetID = assetID
        };
        if (!session.AddObjectComponent(
                id,
                std::move(modelComponent)).isValid())
        {
            static_cast<void>(session.DeleteObject(id));
            SetError(outError, "Failed to attach model renderer component.");
            return false;
        }

        outIDs.clear();
        outIDs.push_back(id);
        return true;
    }

    bool EditorAssetSpawner::TryGetSelectedAssetBounds(
        EditorSession &session,
        Limen::AxisAlignedBoundingBox &outBounds,
        std::string *outError) const
    {
        if (outError)
            outError->clear();

        const Limen::AssetID assetID = session.GetSelectedAssetID();
        if (!assetID.isValid())
        {
            SetError(outError, "No asset is selected.");
            return false;
        }

        Limen::AssetRecord record;
        if (!session.GetAssetRegistry().TryFind(assetID, record))
        {
            SetError(outError, "Selected asset is not registered.");
            return false;
        }

        const std::string logicalPath(
            record.LogicalPath.data(),
            record.LogicalPath.size());
        if (record.Status != Limen::AssetRecordStatus::Present)
        {
            SetError(outError, "Selected asset file is missing.");
            return false;
        }
        if (ToLowerExtension(std::filesystem::path(logicalPath)) != ".obj")
        {
            SetError(outError, "Only OBJ model assets can be placed.");
            return false;
        }

        const Limen::Ref<Limen::Model> model =
            Limen::AssetManager::LoadModel(logicalPath);
        if (!model)
        {
            SetError(outError, "Failed to load the selected model.");
            return false;
        }
        if (!model->GetLocalBounds().IsValid())
        {
            SetError(outError, "Model bounds are unavailable.");
            return false;
        }

        outBounds = model->GetLocalBounds();
        return true;
    }
}
