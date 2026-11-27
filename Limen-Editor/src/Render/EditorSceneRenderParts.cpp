#include "Render/EditorSceneRenderParts.h"

#include <cctype>
#include <filesystem>
#include <string_view>
#include <system_error>
#include <utility>
#include <variant>

#include "Limen/Asset/AssetManager.h"
#include "Limen/Asset/AssetPath.h"
#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Asset/Model.h"
#include "Limen/Asset/ModelMaterialBuilder.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneComponent.h"
#include "Limen/Scene/SceneSerializer.h"
#include "Render/EditorSceneContent.h"

namespace LimenEditor
{
    namespace
    {
        using Limen::SceneFileErrorCode;

        std::string ToLowerExtension(const std::filesystem::path &path)
        {
            std::string extension = path.extension().string();
            for (char &character : extension)
                character = static_cast<char>(std::tolower(static_cast<unsigned char>(character)));
            return extension;
        }
    }

    Limen::Ref<Limen::Model> LoadEditorModelAsset(
        const Limen::AssetRegistry &registry,
        const Limen::AssetID assetID,
        Limen::SceneFileError &error,
        std::string *outLogicalPath)
    {
        const auto fail = [&](SceneFileErrorCode code, std::string_view message)
        {
            error = {
                .Code = code,
                .Key = "model_asset_id",
                .Message = Limen::LString(message) + " (AssetID " + Limen::AssetIDToString(assetID) + ")"
            };
            return Limen::Ref<Limen::Model>{};
        };

        Limen::AssetRecord record;
        if (!assetID.isValid() || !registry.TryFind(assetID, record))
            return fail(SceneFileErrorCode::InvalidID, "Model asset is not registered.");

        std::string logicalPath(record.LogicalPath.data(), record.LogicalPath.size());
        if (record.Status != Limen::AssetRecordStatus::Present)
            return fail(SceneFileErrorCode::FileAccess, "Model asset file is missing: " + logicalPath);

        const std::filesystem::path assetPath = Limen::AssetPath::Resolve(
            std::filesystem::path(std::u8string(logicalPath.begin(), logicalPath.end())));
        if (ToLowerExtension(assetPath) != ".obj")
            return fail(SceneFileErrorCode::InvalidField, "Only OBJ model assets are supported: " + logicalPath);

        // 资源文件可能在注册表加载之后被移除。先检查磁盘，避免模型缓存掩盖失效引用。
        std::error_code fileError;
        if (!std::filesystem::is_regular_file(assetPath, fileError))
            return fail(SceneFileErrorCode::FileAccess, "Model asset file is unavailable: " + logicalPath);

        auto model = Limen::AssetManager::LoadModelFromFile(assetPath);
        if (!model)
            return fail(SceneFileErrorCode::InstantiatedFailed, "Failed to load model asset: " + logicalPath);

        if (outLogicalPath)
            *outLogicalPath = std::move(logicalPath);
        error = {};
        return model;
    }

    bool TryResolveEditorModelParts(
        const Limen::AssetRegistry &registry,
        const EditorSceneContent &sceneContent,
        const Limen::AssetID assetID,
        Limen::LArray<Limen::SceneRenderObject> &outParts,
        Limen::SceneFileError &error,
        std::string *outLogicalPath)
    {
        std::string logicalPath;
        const auto model = LoadEditorModelAsset(registry, assetID, error, &logicalPath);
        if (!model)
            return false;

        const auto &pipeline = sceneContent.GetLitPipeline();
        if (!pipeline)
        {
            error = {
                .Code = SceneFileErrorCode::InstantiatedFailed,
                .Message = "Editor render pipeline is unavailable."
            };
            return false;
        }

        const auto materials = Limen::ModelMaterialBuilder::BuildBlinnPhong(*model, pipeline);
        const auto &modelParts = model->GetParts();
        Limen::LArray<Limen::SceneRenderObject> parts;
        parts.reserve(modelParts.getSize());
        for (const auto &part : modelParts)
        {
            if (!part.MeshResource)
                continue;

            // 模型未指定源材质时，使用 Editor 默认材质。
            const auto &material = part.MaterialSlot < materials.getSize()
                ? materials[part.MaterialSlot] : sceneContent.GetCubeMaterial();
            if (material)
                parts.push_back({part.MeshResource, material, part.LocalTransform});
        }

        if (parts.empty())
        {
            error = {
                .Code = SceneFileErrorCode::InstantiatedFailed,
                .Key = "model_asset_id",
                .Message = "Model contains no renderable parts."
            };
            return false;
        }

        if (outLogicalPath)
            *outLogicalPath = std::move(logicalPath);
        outParts = std::move(parts);
        return true;
    }

    bool TryResolveEditorPrimitiveParts(
        const EditorSceneContent &sceneContent,
        const Limen::PrimitiveRendererComponent &primitive,
        Limen::LArray<Limen::SceneRenderObject> &outParts,
        Limen::SceneFileError &error)
    {
        if (primitive.Primitive != Limen::PrimitiveType::Cube)
        {
            error = {
                .Code = SceneFileErrorCode::InvalidField,
                .Key = "primitive",
                .Message = "Unsupported primitive type."
            };
            return false;
        }

        const auto &mesh = sceneContent.GetCubeMesh();
        const auto &material = sceneContent.GetCubeMaterial();
        if (!mesh || !material)
        {
            error = {
                .Code = SceneFileErrorCode::InstantiatedFailed,
                .Key = "primitive",
                .Message = "Editor primitive resources are unavailable."
            };
            return false;
        }

        outParts = Limen::LArray<Limen::SceneRenderObject>{{mesh, material}};
        error = {};
        return true;
    }

    bool TryResolveEditorSceneRenderParts(
        const Limen::AssetRegistry &registry,
        const EditorSceneContent &sceneContent,
        const Limen::SceneObjectComponent &component,
        Limen::LArray<Limen::SceneRenderObject> &outParts,
        Limen::SceneFileError &error)
    {
        if (const auto *model = std::get_if<Limen::ModelRendererComponent>(&component.Data))
            return TryResolveEditorModelParts(registry, sceneContent, model->ModelAssetID, outParts, error);

        if (const auto *primitive = std::get_if<Limen::PrimitiveRendererComponent>(&component.Data))
            return TryResolveEditorPrimitiveParts(sceneContent, *primitive, outParts, error);

        // 点光源通过场景的光照组件参与渲染，没有几何部件。
        outParts.clear();
        error = {};
        return true;
    }
}
