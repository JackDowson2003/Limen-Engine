//
// Created by chenlong on 2026/11/24.
//
#include "Limen/Scene/SceneDocument.h"

#include "Limen/Scene/Scene.h"

namespace Limen
{
    SceneDocument CaptureSceneDocument(const Scene &scene, const std::string_view displayName)
    {
        SceneDocument document;
        document.DisplayName = LString(displayName);
        document.Ambient = scene.GetAmbientLight();
        document.DirectionLight = scene.GetDirectionalLight();
        scene.ForEachObjectAuthoring(
        [&document](const SceneObjectAuthoringData& data)
        {
            document.Objects.push_back(data);
        }
        );
        return document;
    }
}
