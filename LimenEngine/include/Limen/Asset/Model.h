//
// Created by chenlong on 2026/9/14.
//

#pragma once

#include <glm/vec3.hpp>
#include <limits>
#include <vector>
#include <cstdint>
#include <string>

#include <glm/mat4x4.hpp>

#include "Limen/Core/Core.h"
#include "Limen/Renderer/Mesh.h"

namespace Limen
{
    /**
     * @brief Model中的一个材质槽定义。
     *
     * 当前只保存源模型中的材质名称。
     * 它不是Renderer使用的Material，也不拥有Shader或Pipeline。
     */
    struct ModelMaterialSlot
    {
        // OBJ/MTL中的 new mtl 名称。
        std::string Name;

        // MTL中的Ka：环境光反射系数。
        glm::vec3 AmbientReflectance{0.0f};

        // MTL中的Kd：漫反射系数。
        glm::vec3 DiffuseReflectance{1.0f};

        // MTL中的Ks：镜面反射系数。
        glm::vec3 SpecularReflectance{0.0f};

        // MTL中的Ns：Blinn-Phong高光指数。
        float Shininess = 32.0f;
    };

    /**
     * @brief 一个Model中可以独立绘制的部分
     *
     * 一个Model 可能由身体、头发、眼睛等多个部分组成
     * 每个部分拥有自己的Mesh、Material Slot 和局部变换
     */
    struct ModelPart
    {
        /**
         * @brief 表示该ModelPart没有对应的导入材质。
         *
         * uint32_t的最大值不会作为正常材质数组下标使用。
         */
        static constexpr uint32_t InvalidMaterialSlot = std::numeric_limits<uint32_t>::max();

        // 模型部分的名字，例如 Body, Hair
        std::string Name;

        // 这个部分实际使用的几何资源
        Ref<Mesh> MeshResource;

        /**
         * 该部分使用的材质槽下标。
         *
         * 数值 2 表示使用模型材质列表中的第 2 号槽位，
         * 不是地址，也不是 GPU 资源 ID。
         */
        uint32_t MaterialSlot = InvalidMaterialSlot;

        // 该部分相对于整个 Model 根节点的局部变换。
        glm::mat4 LocalTransform{1.0f};
    };

    /**
     * @brief 可以由一个 或者多个 ModelPart 组成的完整资源模型
     *
     * Model 只负责拥有和组织模型的各个可绘制部分
     * 它不负责读取 .obj, 也不直接诶发出渲染指令
     */
    class LIMEN_API Model final
    {
    public:
        /**
         * @brief 使用已经创建好的模型部分构造Model
         * std::vector<ModelPart> parts 是可以进行move的，也可以保持之前的，执行复制
         *
         * @param parts
         * ModelImporter 解析源文件后创建的可绘制部分
         *
         * @param materialSlots
         * 源模型中定义的材质槽列表，ModelPart通过MaterialSlot索引该列表。
         */
        explicit Model(std::vector<ModelPart> parts, std::vector<ModelMaterialSlot> materialSlots = {});

        /**
        * @brief 获取模型包含的全部可绘制部分。
        *
        * 返回 const 引用，避免复制整个数组，同时禁止调用者
        * 绕过 Model 添加、删除或替换内部部分。
        */
        [[nodiscard]]
        const std::vector<ModelPart> &GetParts() const noexcept;

        /**
         * @brief 获取模型的材质槽列表。
         */
        [[nodiscard]]
        const std::vector<ModelMaterialSlot> &GetMaterialSlots() const noexcept;

    private:
        // 组成当前 model 的每个部分
        std::vector<ModelPart> m_Parts;

        // 每个ModelPart 的Material Slot
        std::vector<ModelMaterialSlot> m_MaterialSlots;
    };
}
