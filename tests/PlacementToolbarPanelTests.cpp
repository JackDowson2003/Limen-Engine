#include <chrono>
#include <iostream>

#include "UI/Panels/PlacementToolbarPanel.h"

namespace
{
    bool Check(const char *name, const bool passed)
    {
        if (!passed)
            std::cerr << "FAIL: " << name << '\n';
        return passed;
    }
}

int main()
{
    using Clock = std::chrono::steady_clock;
    using Action = LimenEditor::PlacementToolbarAction;

    const LimenEditor::UIRect topBar{
        0.0f,
        0.0f,
        1600.0f,
        128.0f
    };
    const LimenEditor::PlacementToolbarLayout layout =
        LimenEditor::CalculatePlacementToolbarLayout(topBar, 36.0f);

    bool passed = Check("placement label has positive size",
                        layout.Label.Width > 0.0f &&
                        layout.Label.Height > 0.0f);
    passed &= Check("cube button has positive size",
                    layout.CubeButton.Width > 0.0f &&
                    layout.CubeButton.Height > 0.0f);
    passed &= Check("point light button has positive size",
                    layout.PointLightButton.Width > 0.0f &&
                    layout.PointLightButton.Height > 0.0f);
    passed &= Check("placement buttons do not overlap",
                    layout.CubeButton.X + layout.CubeButton.Width <=
                    layout.PointLightButton.X);
    passed &= Check("placement controls stay in top bar",
                    layout.PointLightButton.X +
                        layout.PointLightButton.Width <=
                    topBar.Width &&
                    layout.PointLightButton.Y +
                        layout.PointLightButton.Height <=
                    topBar.Height);

    LimenEditor::PlacementToolbarPanel panel;
    panel.UpdateLayout(topBar, 36.0f);
    const Clock::time_point start{};
    const float cubeX =
        layout.CubeButton.X + layout.CubeButton.Width * 0.5f;
    const float buttonY =
        layout.CubeButton.Y + layout.CubeButton.Height * 0.5f;
    passed &= Check("cube button emits placement intent",
                    panel.OnMousePressed(cubeX, buttonY, start) ==
                    Action::PlaceCube);
    passed &= Check("rapid repeated cube press is ignored",
                    panel.OnMousePressed(
                        cubeX,
                        buttonY,
                        start + std::chrono::milliseconds(100)) ==
                    Action::None);
    passed &= Check("cube button recovers after debounce interval",
                    panel.OnMousePressed(
                        cubeX,
                        buttonY,
                        start + std::chrono::milliseconds(351)) ==
                    Action::PlaceCube);

    const float pointLightX =
        layout.PointLightButton.X +
        layout.PointLightButton.Width * 0.5f;
    passed &= Check("point light button emits placement intent",
                    panel.OnMousePressed(
                        pointLightX,
                        buttonY,
                        start + std::chrono::milliseconds(400)) ==
                    Action::PlacePointLight);
    passed &= Check("click outside toolbar emits nothing",
                    panel.OnMousePressed(
                        500.0f,
                        100.0f,
                        start + std::chrono::milliseconds(800)) ==
                    Action::None);

    const LimenEditor::PlacementToolbarLayout tooSmall =
        LimenEditor::CalculatePlacementToolbarLayout(
            {0.0f, 0.0f, 120.0f, 80.0f},
            36.0f);
    passed &= Check("narrow top bar leaves controls hidden",
                    tooSmall.CubeButton.Width == 0.0f &&
                    tooSmall.PointLightButton.Width == 0.0f);

    if (passed)
        std::cout << "Placement toolbar: PASS\n";
    return passed ? 0 : 1;
}
