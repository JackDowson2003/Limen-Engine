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

        // 没有Albedo纹理时使用共享白纹理，保持材质颜色不变。
        const Ref<Texture2D> &whiteTexture = AssetManager::GetWhiteTexture();
        if (!whiteTexture)
            return {};

        // 没有Normal Map时使用共享平坦法线纹理，近似保持原始法线方向。
        const Ref<Texture2D> &flatNormalTexture = AssetManager::GetFlatNormalTexture();

        if (!flatNormalTexture)
            return {};

        for (const ModelMaterialSlot &slot: materialSlots)
        {
            Ref<Material> material = CreateRef<Material>(
                pipeline,
                "Imported Material: " + slot.Name
            );

            // 默认使用白纹理。
            Ref<Texture2D> albedoTexture = whiteTexture;

            // map_Kd存在时，尝试用文件纹理替换默认白纹理。
            if (!slot.AlbedoTexturePath.empty())
            {
                Ref<Texture2D> loadedTexture = AssetManager::LoadTexture2DFromFile(slot.AlbedoTexturePath,TextureColorSpace::SRGB);

                // 加载成功则替换
                if (loadedTexture)
                    albedoTexture = std::move(loadedTexture);
            }
            material->SetTexture("u_AlbedoTexture", albedoTexture, 0);

            // 默认使用近似保持原始法线方向的平坦法线纹理。
            Ref<Texture2D> normalTexture = flatNormalTexture;

            if (!slot.NormalTexturePath.empty())
            {
                Ref<Texture2D> loadedTexture = AssetManager::LoadTexture2DFromFile(slot.NormalTexturePath, TextureColorSpace::Linear);

                // 加载成功时替换平坦法线纹理；失败时继续使用回退资源。
                if (loadedTexture)
                    normalTexture = std::move(loadedTexture);
            }
            material->SetTexture("u_NormalTexture", normalTexture, 1);

            material->SetFloat3("u_AmbientReflectance", slot.AmbientReflectance);
            material->SetFloat3("u_DiffuseReflectance", slot.DiffuseReflectance);
            material->SetFloat3("u_SpecularColor", slot.SpecularReflectance);
            material->SetFloat("u_Shininess", slot.Shininess);
            materials.push_back(std::move(material));
        }


        return materials;
    }
}
