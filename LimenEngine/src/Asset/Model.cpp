//
// Created by chenlong on 2026/9/14.
//

#include "Limen/Asset/Model.h"

#include <utility>

#include "Limen/Core/Log.h"

namespace Limen
{
    Model::Model(std::vector<ModelPart> parts, std::vector<ModelMaterialSlot> materialSlots)
        : m_Parts(std::move(parts)), m_MaterialSlots(std::move(materialSlots))
    {
        // 一个有效的 Model 至少应当包含一个可绘制部分。
        LM_CORE_ASSERT(
            !m_Parts.empty(),
            "Cannot create Model without any parts"
        );

        // 每个 ModelPart 都必须拥有有效的 Mesh。
        for (const auto &[Name
                 , MeshResource
                 , MaterialSlot
                 , LocalTransform]: m_Parts)
        {
            LM_CORE_ASSERT(
                MeshResource,
                "Model part '{}' does not contain a valid Mesh",
                Name
            );

            if (!MeshResource)
                continue;

            /*
             * Mesh保存的是Part局部空间AABB。
             *
             * 先通过LocalTransform把它转换到Model局部空间，
             * 再合并到整个Model的包围盒中。
            */
            const AxisAlignedBoundingBox partBounds =
                    MeshResource->GetLocalBounds().Transformed(LocalTransform);

            m_LocalBounds.Expand(partBounds);

            /*
             * 没有材质的Part允许使用InvalidMaterialSlot；
             * 有材质时，下标必须位于材质槽列表中。
             */
            LM_CORE_ASSERT(
                MaterialSlot == ModelPart::InvalidMaterialSlot ||
                MaterialSlot < m_MaterialSlots.size(),
                "Model part '{}' contains invalid material slot {}",
                Name,
                MaterialSlot
            );
        }

        LM_CORE_ASSERT(
            m_LocalBounds.IsValid(),
            "Failed to calculate local bounds for Model"
        );

    }

    const std::vector<ModelPart> &Model::GetParts() const noexcept
    {
        return m_Parts;
    }

    const AxisAlignedBoundingBox &Model::GetLocalBounds() const noexcept
    {
        return m_LocalBounds;
    }

    const std::vector<ModelMaterialSlot> &Model::GetMaterialSlots() const noexcept
    {
        return m_MaterialSlots;
    }
}
