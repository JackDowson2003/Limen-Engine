#pragma once

#include <glm/vec4.hpp>

namespace LimenEditor
{
    // 中性石墨色区分工作区，琥珀色标记当前面板与选中项。
    struct EditorTheme
    {
        glm::vec4 WindowBackground{0.073f, 0.073f, 0.076f, 1.0f};
        glm::vec4 TopBarBackground{0.105f, 0.107f, 0.110f, 1.0f};
        glm::vec4 PanelBackground{0.129f, 0.132f, 0.137f, 1.0f};
        glm::vec4 ViewportBackground{0.095f, 0.103f, 0.112f, 1.0f};
        glm::vec4 Splitter{0.103f, 0.105f, 0.108f, 1.0f};
        glm::vec4 PanelBorder{0.202f, 0.206f, 0.212f, 1.0f};
        glm::vec4 ViewportOverlayBackground{0.071f, 0.077f, 0.084f, 0.94f};

        glm::vec4 TextPrimary{0.94f, 0.94f, 0.93f, 1.0f};
        glm::vec4 TextSecondary{0.76f, 0.77f, 0.79f, 1.0f};
        glm::vec4 TextMuted{0.58f, 0.60f, 0.63f, 1.0f};

        glm::vec4 Accent{0.94f, 0.70f, 0.42f, 1.0f};
        glm::vec4 SelectionBackground{0.29f, 0.23f, 0.17f, 1.0f};
        glm::vec4 SelectionAccent{0.97f, 0.76f, 0.50f, 1.0f};
        glm::vec4 SelectionText{0.98f, 0.93f, 0.85f, 1.0f};
        glm::vec4 RowHoverBackground{0.18f, 0.19f, 0.20f, 1.0f};
        glm::vec4 MissingText{0.98f, 0.60f, 0.54f, 1.0f};

        glm::vec4 ButtonBackground{0.162f, 0.165f, 0.171f, 1.0f};
        glm::vec4 ButtonHoverBackground{0.225f, 0.227f, 0.232f, 1.0f};
        glm::vec4 ButtonText{0.89f, 0.90f, 0.91f, 1.0f};

        float FontPixelHeight = 19.0f;
        float PanelPadding = 14.0f;
        float PanelHeaderHeight = 40.0f;
        float RowSpacing = 8.0f;
        float SectionSpacing = 12.0f;
    };
}
