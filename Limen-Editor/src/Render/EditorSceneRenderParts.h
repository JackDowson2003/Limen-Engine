#pragma once

#include <string>

#include "Limen/Asset/AssetID.h"
#include "Limen/Core/Core.h"
#include "Limen/Tool/LArray.h"

namespace Limen
{
    class AssetRegistry;
    class Model;
    struct PrimitiveRendererComponent;
    struct SceneFileError;
    struct SceneObjectComponent;
    struct SceneRenderObject;
}

namespace LimenEditor
{
    class EditorSceneContent;

    [[nodiscard]] Limen::Ref<Limen::Model> LoadEditorModelAsset(
        const Limen::AssetRegistry &registry,
        Limen::AssetID assetID,
        Limen::SceneFileError &error,
        std::string *outLogicalPath = nullptr);

    [[nodiscard]] bool TryResolveEditorPrimitiveParts(
        const EditorSceneContent &sceneContent,
        const Limen::PrimitiveRendererComponent &primitive,
        Limen::LArray<Limen::SceneRenderObject> &outParts,
        Limen::SceneFileError &error);

    [[nodiscard]] bool TryResolveEditorModelParts(
        const Limen::AssetRegistry &registry,
        const EditorSceneContent &sceneContent,
        Limen::AssetID assetID,
        Limen::LArray<Limen::SceneRenderObject> &outParts,
        Limen::SceneFileError &error,
        std::string *outLogicalPath = nullptr);

    // 解析成功后交付组件空间的绘制项，失败时保留原输出。
    // error.Key 相对于所属组件，由场景加载流程补齐完整路径。
    [[nodiscard]] bool TryResolveEditorSceneRenderParts(
        const Limen::AssetRegistry &registry,
        const EditorSceneContent &sceneContent,
        const Limen::SceneObjectComponent &component,
        Limen::LArray<Limen::SceneRenderObject> &outParts,
        Limen::SceneFileError &error);
}
