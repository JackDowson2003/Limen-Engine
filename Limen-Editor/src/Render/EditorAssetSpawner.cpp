#include "Render/EditorAssetSpawner.h"

#include <filesystem>
#include <string>
#include <string_view>
#include <utility>

#include "Limen/Asset/Model.h"
#include "Limen/Core/Log.h"
#include "Limen/Scene/SceneSerializer.h"
#include "Project/EditorSession.h"
#include "Render/EditorObjectNaming.h"
#include "Render/EditorSceneRenderParts.h"

namespace LimenEditor
{
    namespace
    {
        void SetError(std::string *outError, const std::string_view message)
        {
            if (outError)
                *outError = std::string(message);
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

        std::string logicalPath;
        Limen::LArray<Limen::SceneRenderObject> parts;
        Limen::SceneFileError error;
        if (!TryResolveEditorModelParts(session.GetAssetRegistry(), sceneContent, assetID,
                                        parts, error, &logicalPath))
        {
            LM_WARN("Editor spawn: model asset {}: {}", Limen::AssetIDToString(assetID), error.Message);
            SetError(outError, error.Message);
            return false;
        }

        // 一个模型实例 = 一个 SceneObject + 多个渲染组件。
        const auto name = std::filesystem::path(
            std::u8string(logicalPath.begin(), logicalPath.end())).stem().u8string();
        const std::string label = MakeUniqueObjectLabel(
            session, std::string(name.begin(), name.end()));
        const Limen::SceneObjectID id =
            session.CreateRenderableObjectGroup(
                parts,
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

        Limen::SceneFileError error;
        const auto model = LoadEditorModelAsset(session.GetAssetRegistry(), assetID, error);
        if (!model)
        {
            SetError(outError, error.Message);
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
