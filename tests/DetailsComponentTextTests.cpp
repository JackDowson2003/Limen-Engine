#include <iostream>
#include <string>
#include <variant>

#include "Limen/Scene/SceneComponent.h"
#include "UI/Panels/DetailsComponentText.h"

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
    bool passed = true;

    Limen::SceneObjectComponent model;
    model.ID = {1, 2};
    model.Data = Limen::ModelRendererComponent{
        .ModelAssetID = {
            0x0011223344556677ULL,
            0x8899aabbccddeeffULL
        }
    };
    const LimenEditor::DetailsComponentDescription modelRow =
        LimenEditor::DescribeSceneObjectComponent(
            model,
            "models/TestCube.obj");
    passed &= Check("model title is shown",
                    modelRow.Title == "Model Renderer");
    passed &= Check("model logical path is preferred",
                    modelRow.Summary == "models/TestCube.obj");

    const LimenEditor::DetailsComponentDescription modelFallback =
        LimenEditor::DescribeSceneObjectComponent(model);
    passed &= Check("unresolved model falls back to AssetID",
                    modelFallback.Summary ==
                    "00112233-4455-6677-8899-aabbccddeeff");

    Limen::SceneObjectComponent primitive;
    primitive.ID = {3, 4};
    primitive.Data = Limen::PrimitiveRendererComponent{
        .Primitive = Limen::PrimitiveType::Cube
    };
    const LimenEditor::DetailsComponentDescription primitiveRow =
        LimenEditor::DescribeSceneObjectComponent(primitive);
    passed &= Check("primitive title is shown",
                    primitiveRow.Title == "Primitive Renderer");
    passed &= Check("cube primitive type is shown",
                    primitiveRow.Summary == "Cube");

    Limen::SceneObjectComponent light;
    light.ID = {5, 6};
    light.Data = Limen::PointLightComponent{
        .Color = {1.0f, 0.5f, 0.25f},
        .Intensity = 4.0f
    };
    const LimenEditor::DetailsComponentDescription lightRow =
        LimenEditor::DescribeSceneObjectComponent(light);
    passed &= Check("point light title is shown",
                    lightRow.Title == "Point Light");
    passed &= Check("point light color is shown",
                    lightRow.Summary ==
                    "Color 1.00 0.50 0.25");
    passed &= Check("point light intensity is shown separately",
                    lightRow.Detail == "Intensity 4.00");

    if (passed)
        std::cout << "Details component text: PASS\n";
    return passed ? 0 : 1;
}
