//
// Created by chenlong on 2026/9/17.
//

#include "Limen/Asset/ModelMaterialBuilder.h"

#include <utility>

#include "Limen/Asset/AssetManager.h"
#include "Limen/RHI/Texture.h"
#include "Limen/Asset/Model.h"
#include "Limen/Renderer/Material.h"
#include "Limen/RHI/GraphicsPipeline.h"

namespace Limen
{
    std::vector<Ref<Material> > ModelMaterialBuilder::BuildBlinnPhong(const Model &model, const Ref<GraphicsPipeline> &pipeline)
    {
        if (!pipeline)
            return {};

        const std::vector<ModelMaterialSlot> &materialSlots = model.GetMaterialSlots();

        std::vector<Ref<Material> > materials;
        materials.reserve(materialSlots.size());

        // Get default white texture
        const Ref<Texture2D> &whiteTexture = AssetManager::GetWhiteTexture();
        if (!whiteTexture)
            return {};

        for (const ModelMaterialSlot &slot: materialSlots)
        {
            Ref<Material> material = CreateRef<Material>(
                pipeline,
                "Imported Material: " + slot.Name
            );

            // 默认使用白纹理。
            Ref<Texture2D> albedoTexture = whiteTexture;

            // 代表有map_Kd，则需要使用文件纹理
            if (!slot.AlbedoTexturePath.empty())
            {
                Ref<Texture2D> loadedTexture = AssetManager::LoadTexture2DFromFile(slot.AlbedoTexturePath);

                // 加载成功则替换
                if (loadedTexture)
                    albedoTexture = std::move(loadedTexture);
            }
            material->SetTexture("u_AlbedoTexture", albedoTexture, 0);

            material->SetFloat3("u_AmbientReflectance", slot.AmbientReflectance);
            material->SetFloat3("u_DiffuseReflectance", slot.DiffuseReflectance);
            material->SetFloat3("u_SpecularColor", slot.SpecularReflectance);
            material->SetFloat("u_Shininess", slot.Shininess);
            materials.push_back(std::move(material));
        }


        return materials;
    }
}
