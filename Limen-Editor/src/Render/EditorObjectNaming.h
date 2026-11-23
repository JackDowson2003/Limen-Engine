#pragma once

#include <cstdint>
#include <string>
#include <string_view>

#include "Project/EditorSession.h"

namespace LimenEditor
{
    // Editor 负责生成唯一显示名称；Scene 本身只保存 Label。
    [[nodiscard]] inline bool IsObjectLabelInUse(
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

    [[nodiscard]] inline std::string MakeUniqueObjectLabel(
        const EditorSession &session,
        const std::string_view base)
    {
        const std::string safeBase =
            base.empty() ? std::string{"Object"} : std::string{base};
        if (!IsObjectLabelInUse(session, safeBase))
            return safeBase;

        for (uint32_t suffix = 2;; ++suffix)
        {
            std::string candidate =
                safeBase + "_" + std::to_string(suffix);
            if (!IsObjectLabelInUse(session, candidate))
                return candidate;
        }
    }
}
