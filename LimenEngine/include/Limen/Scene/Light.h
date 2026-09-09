//
// Created by chenlong on 2026/9/7.
//

#pragma once

#include <glm/vec3.hpp>

namespace Limen
{
    /**
     * @brief 场景中的平行光数据
     *
     * 平行光没有具体位置，所有光线方向相同
     * 可以近似太阳等距离场景非常远的光源
     * DirectionalLight 只保存跨图形 API 的数据场景，
     * And
     * 不包括 Shader、OpenGL、DirectX 或者 Metal 接口
     */
    struct DirectionalLight
    {
        /**
         * @brief 光线从光源射向场景的方向。
         *
         * 约定：
         * Direction 表示光传播方向；
         * Shader 中从表点指向光源的方向 l 应使用 -Direction。
         *
         * 当前默认值对应之前 Shader 中：
         * l = normalize(vec3(-1.0, 1.0, 1.0))
         */
        glm::vec3 Direction{1.f, -1.f, -1.f};

        /**
         * @brief 光源颜色，使用线性 RGB。
         *
         * 它表示各颜色通道的相对比例，
         * 不包含光源整体亮度。
         */
        glm::vec3 Color{
            1.0f,
            0.95f,
            0.85f
        };

        /**
        * @brief 光源强度倍率。
        *
        * 最终近似光强为 Color * Intensity。
        * 当前还不是具有真实物理单位的光度值。
        */
        float Intensity = 1.0f;
    };

    /**
     * @brief 点光源
     *
     * 点光源具有确定的世界坐标位置，
     * 光线从该位置向四周传播，并随距离增加而衰减
     */
    struct PointLight
    {
        // 点光源在世界中的位置
        glm::vec3 Position{0.f};

        // 点光源的线性 RGB 颜色
        glm::vec3 Color{1.f};

        // 光源的亮度倍率
        float Intensity = 1.0f;

    };

    /**
     * @brief 常量环境光
     *
     * 它用于近似场景中来自四面八方的间接光照，
     * 不具有位置或方向。
     *
     * 这并不是真实的环境贴图光照或IBL，
     * 只是GAMES101局部光照模型中的常量I_a。
     *
     */
    struct AmbientLight
    {
        // 环境光的线性 RGB 颜色
        glm::vec3 Color{1.f};

        /**
         * 环境光的整体强度倍率
         *
         * 默认值为 1，可以保持当前 Shader 中的结果：
         *
         * k_a = 0.15 * k_d
         * I_a = vec3(1.0)
         * L_a = k_a * I_a
         */
        float Intensity = 1.0f;
    };
}
