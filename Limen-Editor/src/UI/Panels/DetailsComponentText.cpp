#include "UI/Panels/DetailsComponentText.h"

#include <iomanip>
#include <sstream>
#include <variant>

#include "Limen/Asset/AssetID.h"
#include "Limen/Scene/SceneComponent.h"

namespace LimenEditor
{
    namespace
    {
        std::string FormatFloat(const float value)
        {
            std::ostringstream output;
            output << std::fixed << std::setprecision(2) << value;
            return output.str();
        }
    }

    DetailsComponentDescription DescribeSceneObjectComponent(
        const Limen::SceneObjectComponent &component,
        const std::string_view modelAssetDisplayName)
    {
        if (const auto *model =
                std::get_if<Limen::ModelRendererComponent>(
                    &component.Data))
        {
            const Limen::LString assetID =
                Limen::AssetIDToString(model->ModelAssetID);
            return {
                .Title = "Model Renderer",
                .Summary = modelAssetDisplayName.empty()
                               ? std::string(
                                     assetID.data(),
                                     assetID.size())
                               : std::string(modelAssetDisplayName)
            };
        }

        if (const auto *primitive =
                std::get_if<Limen::PrimitiveRendererComponent>(
                    &component.Data))
        {
            return {
                .Title = "Primitive Renderer",
                .Summary = primitive->Primitive ==
                                   Limen::PrimitiveType::Cube
                               ? "Cube"
                               : "Unknown"
            };
        }

        const auto &light =
            std::get<Limen::PointLightComponent>(component.Data);
        return {
            .Title = "Point Light",
            .Summary =
                "Color " + FormatFloat(light.Color.x) + " " +
                FormatFloat(light.Color.y) + " " +
                FormatFloat(light.Color.z),
            .Detail = "Intensity " + FormatFloat(light.Intensity)
        };
    }
}
