//
// Created by chenlong on 2026/9/17.
//

#pragma once

#include <limits>

#include <glm/common.hpp>
#include <glm/vec3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>

namespace Limen
{
    /**
     * @brief 轴对齐包围盒
     *
     * 使用局部坐标系中的最小点和最大点
     * 描述一组顶点所占据的空间范围
     */
    struct AxisAlignedBoundingBox
    {
        /*
         * 默认构造时创建一个无效的空包围盒。
         *
         * 第一次调用 Expand() 后，
         * Min 和 Max 都会被正确更新到第一个点的位置。
         */
        glm::vec3 Min{
            std::numeric_limits<float>::max(),
        };


        glm::vec3 Max{
            std::numeric_limits<float>::lowest(),
        };

        /**
         * @brief 把一个点加入当前包围盒。
         *
         * 分别更新X、Y、Z方向上的最小值和最大值。
         */
        void Expand(const glm::vec3 &point) noexcept
        {
            Min = glm::min(Min, point);
            Max = glm::max(Max, point);
        }

        /**
        * @brief 判断当前包围盒是否至少包含过一个有效点。
        */
        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return Min.x <= Max.x &&
                   Min.y <= Max.y &&
                   Min.z <= Max.z;
        }

        /**
         * @brief 将另一个有效包围盒合并到当前包围盒。
         *
         * 两个包围盒必须处于同一个坐标空间。
         * 无效的空包围盒不会影响当前结果。
         */
        void Expand(const AxisAlignedBoundingBox &bounds) noexcept
        {
            if (!bounds.IsValid())
                return;

            Expand(bounds.Min);
            Expand(bounds.Max);
        }

        /**
         *
         *  @param transform 从当前包围盒所在坐标空间到目标坐标空间的变换矩阵
         *  @return 目标坐标空间中的新AABB
         */
        [[nodiscard]]
        AxisAlignedBoundingBox Transformed(const glm::mat4 &transform) const noexcept
        {
            AxisAlignedBoundingBox transformedBounds;

            if (!IsValid())
                return transformedBounds;

            // AABB总共8个角，X可以选择Min.x or Max.x(y, z 同理)
            // 2 x 2 x 2 = 8
            for (int x = 0; x < 2; ++x)
            {
                for (int y = 0; y < 2; ++y)
                {
                    for (int z = 0; z < 2; ++z)
                    {
                        const glm::vec3 corner
                        {
                            x == 0 ? Min.x : Max.x,
                            y == 0 ? Min.y : Max.y,
                            z == 0 ? Min.z : Max.z
                        };

                        // w = 1, 表示这是一个位置，因此平移分量会生效
                        // 将AABB的每一个角点从当前坐标空间变换到目标坐标空间。
                        const glm::vec4 transformedCorner = transform * glm::vec4(corner, 1.0f);

                        transformedBounds.Expand(glm::vec3(transformedCorner));
                    }
                }
            }
            return transformedBounds;
        }

        /**
        * @brief 获取包围盒中心。
        */
        [[nodiscard]]
        glm::vec3 GetCenter() const noexcept
        {
            return (Min + Max) * 0.5f;
        }

        /**
         * @brief 获取中心到盒子边界的半尺寸。
         *
         * 例如完整宽度是10，那么Extent.x就是5。
         */
        [[nodiscard]]
        glm::vec3 GetExtents() const noexcept
        {
            return (Max - Min) * 0.5f;
        }
    };
}
