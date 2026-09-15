//
// Created by chenlong on 2026/9/14.
//

#include "Limen/Asset/Model.h"

#include <utility>

#include "Limen/Core/Log.h"

namespace Limen
{
    Model::Model(std::vector<ModelPart> parts, std::vector<ModelMaterialSlot> materialSlots)
        :m_Parts(std::move(parts)),m_MaterialSlots(std::move(materialSlots))
    {
        // 一个有效的 Model 至少应当包含一个可绘制部分。
        LM_CORE_ASSERT(
            !m_Parts.empty(),
            "Cannot create Model without any parts"
        );

        // 每个 ModelPart 都必须拥有有效的 Mesh。
        for (const ModelPart& part : m_Parts)
        {
            LM_CORE_ASSERT(
                part.MeshResource,
                "Model part '{}' does not contain a valid Mesh",
                part.Name
            );

            /*
             * 没有材质的Part允许使用InvalidMaterialSlot；
             * 有材质时，下标必须位于材质槽列表中。
             */
            LM_CORE_ASSERT(
                part.MaterialSlot == ModelPart::InvalidMaterialSlot ||
                part.MaterialSlot < m_MaterialSlots.size(),
                "Model part '{}' contains invalid material slot {}",
                part.Name,
                part.MaterialSlot
            );
        }
    }

    const std::vector<ModelPart> & Model::GetParts() const noexcept
    {
        return m_Parts;
    }

    const std::vector<ModelMaterialSlot> & Model::GetMaterialSlots() const noexcept
    {
        return m_MaterialSlots;
    }
}
