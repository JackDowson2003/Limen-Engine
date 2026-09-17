//
// Created by chenlong on 2026/9/17.
//

#pragma once

#include <vector>

#include "Limen/Core/Core.h"

namespace Limen
{
    class GraphicsPipeline;
    class Material;
    class Model;

    /**
     * @brief 将 Model 中的导入材质描述转换为运行时 Material
     *
     * Model 保存的是来自 OBJ/MTL的Ka, Kd, Ks, Ns and map_Kd
     * Material 保存的是Pipeline, Shader参数和纹理绑定
     *
     * 这个类负责链接二者，但是不负责绘制模型
     */
    class LIMEN_API ModelMaterialBuilder final
    {
    public:
        // 当前类没有实例状态，只提供材质转换功能。
        ModelMaterialBuilder() = delete;

        /**
         * @brief 为Model中的所有材质槽创建运行时材质
         *
         * 返回数组的下标与ModelMaterialSlot下标保持一致
         * 例如返回值[2]对应Model中的MaterialSlot 2
         *
         * @param model
         * 包含导入材质描述的模型。
         *
         * @param pipeline
         * 新建Material共同使用的图形管线。
         *
         * @return
         * 与Model材质槽顺序一致的运行时Material数组。
         */
        [[nodiscard]]
        static std::vector<Ref<Material>> BuildBlinnPhong(const Model& model, const Ref<GraphicsPipeline>& pipeline);
    };
}