#pragma once

#include <string>

#include "Limen/Core/Core.h"
#include "Limen/Scene/Scene.h"
#include "Limen/Scene/SceneTransform.h"
#include "Limen/Tool/LArray.h"

namespace LimenEditor
{
    class EditorSession;
    class EditorSceneContent;

    // 创建非资产驱动的 Editor 基础对象。
    // 运行期资源由 EditorSceneContent 提供，Authoring 数据由 SceneComponent 保存。
    class EditorPrimitiveSpawner final
    {
    public:
        [[nodiscard]]
        bool SpawnCube(
            EditorSession &session,
            const EditorSceneContent &sceneContent,
            const Limen::SceneTransform &transform,
            Limen::LArray<Limen::SceneObjectID> &outIDs,
            std::string *outError = nullptr
        ) const;

        [[nodiscard]]
        bool SpawnPointLight(
            EditorSession &session,
            const Limen::SceneTransform &transform,
            Limen::LArray<Limen::SceneObjectID> &outIDs,
            std::string *outError = nullptr
        ) const;
    };
}
