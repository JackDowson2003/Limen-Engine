#pragma once

#include <string>

#include "glm/mat4x4.hpp"

#include "Limen/Core/Core.h"
#include "Limen/Math/AxisAlignedBoundingBox.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Tool/LArray.h"

namespace LimenEditor
{
    class EditorSession;
    class EditorSceneContent;

    // 把 Content Browser 中选中的模型资产实例化为可渲染 SceneObject。
    // 只负责资产→对象转换，不拥有 Scene 或 GPU 资源。
    class EditorAssetSpawner final
    {
    public:
        // 成功时把创建的 SceneObjectID 写入 outIDs，并选中全部新对象。
        bool SpawnSelectedAsset(
            EditorSession &session,
            const EditorSceneContent &sceneContent,
            const glm::mat4 &objectTransform,
            Limen::LArray<Limen::SceneObjectID> &outIDs,
            std::string *outError = nullptr
        ) const;

        // 读取当前选中模型的局部空间包围盒；失败时不修改 outBounds。
        [[nodiscard]]
        bool TryGetSelectedAssetBounds(
            EditorSession &session,
            Limen::AxisAlignedBoundingBox &outBounds,
            std::string *outError = nullptr
        ) const;
    };
}
