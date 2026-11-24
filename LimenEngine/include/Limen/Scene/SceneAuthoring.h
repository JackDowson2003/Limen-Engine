//
// Created by chenlong on 2026/11/23.
//

#pragma once

#include "Limen/Scene/SceneComponent.h"
#include "Limen/Scene/SceneIdentity.h"
#include "Limen/Scene/SceneTransform.h"
#include "Limen/Tool/LArray.h"
#include "Limen/Tool/LString.h"

namespace Limen
{
    // 可持久化的对象快照，不包含 Handle, Generation 或运行期的GPU 资源
    struct SceneObjectAuthoringData
    {
        SceneObjectID ID{};
        LString Label;
        SceneTransform Transform{};
        LArray<SceneObjectComponent> Components;
    };
}