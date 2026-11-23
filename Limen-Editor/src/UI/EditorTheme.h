#pragma once

#include <glm/vec4.hpp>

namespace LimenEditor
{
    // UE5 暗色编辑器视觉基线：近黑工作区、分层灰色面板和单一蓝色强调色。
    struct EditorTheme
    {
        glm::vec4 WindowBackground{0.055f, 0.055f, 0.060f, 1.0f};
        glm::vec4 TopBarBackground{0.078f, 0.078f, 0.084f, 1.0f};
        glm::vec4 PanelBackground{0.105f, 0.105f, 0.112f, 1.0f};
        glm::vec4 PanelHeaderBackground{0.138f, 0.138f, 0.145f, 1.0f};
        glm::vec4 SectionBackground{0.122f, 0.122f, 0.130f, 1.0f};
        glm::vec4 ViewportBackground{0.066f, 0.066f, 0.072f, 1.0f};
        glm::vec4 Splitter{0.045f, 0.045f, 0.050f, 1.0f};
        glm::vec4 PanelBorder{0.205f, 0.205f, 0.218f, 1.0f};
        glm::vec4 ViewportOverlayBackground{0.025f, 0.025f, 0.030f, 0.88f};

        glm::vec4 TextPrimary{0.90f, 0.90f, 0.92f, 1.0f};
        glm::vec4 TextSecondary{0.68f, 0.68f, 0.72f, 1.0f};
        glm::vec4 TextMuted{0.45f, 0.45f, 0.49f, 1.0f};

        glm::vec4 Accent{0.0f, 0.47f, 0.84f, 1.0f};
        glm::vec4 SelectionBackground{0.0f, 0.31f, 0.58f, 1.0f};
        glm::vec4 SelectionAccent{0.10f, 0.56f, 0.96f, 1.0f};
        glm::vec4 SelectionText{1.0f, 1.0f, 1.0f, 1.0f};
        glm::vec4 MissingText{0.85f, 0.34f, 0.34f, 1.0f};

        glm::vec4 ButtonBackground{0.155f, 0.155f, 0.165f, 1.0f};
        glm::vec4 ButtonHoverBackground{0.235f, 0.235f, 0.250f, 1.0f};
        glm::vec4 ButtonBorder{0.275f, 0.275f, 0.295f, 1.0f};
        glm::vec4 ButtonText{0.92f, 0.92f, 0.94f, 1.0f};

        float FontPixelHeight = 18.0f;
        float PanelPadding = 10.0f;
        float PanelHeaderHeight = 30.0f;
        float HeaderOffset = 4.0f;
        float RowSpacing = 2.0f;
        float SectionSpacing = 8.0f;
    };
}
