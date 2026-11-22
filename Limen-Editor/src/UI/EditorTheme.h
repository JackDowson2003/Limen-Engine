#pragma once

#include <glm/vec4.hpp>

namespace LimenEditor
{
    // UE5 暗色编辑器风格：中性灰面板、蓝色选择行、克制的高亮。
    struct EditorTheme
    {
        glm::vec4 WindowBackground{0.106f, 0.106f, 0.106f, 1.0f};
        glm::vec4 TopBarBackground{0.145f, 0.145f, 0.145f, 1.0f};
        glm::vec4 PanelBackground{0.125f, 0.125f, 0.125f, 1.0f};
        glm::vec4 PanelHeaderBackground{0.157f, 0.157f, 0.157f, 1.0f};
        glm::vec4 ViewportBackground{0.078f, 0.078f, 0.078f, 1.0f};
        glm::vec4 Splitter{0.196f, 0.196f, 0.196f, 1.0f};

        glm::vec4 TextPrimary{0.82f, 0.82f, 0.82f, 1.0f};
        glm::vec4 TextSecondary{0.63f, 0.63f, 0.63f, 1.0f};
        glm::vec4 TextMuted{0.44f, 0.44f, 0.44f, 1.0f};

        glm::vec4 SelectionBackground{0.0f, 0.36f, 0.78f, 1.0f};
        glm::vec4 SelectionText{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 MissingText{0.85f, 0.34f, 0.34f, 1.0f};

        glm::vec4 ButtonBackground{0.22f, 0.22f, 0.22f, 1.0f};
        glm::vec4 ButtonText{0.88f, 0.88f, 0.88f, 1.0f};

        float PanelPadding = 12.0f;
        float HeaderOffset = 8.0f;
        float RowSpacing = 4.0f;
    };
}
