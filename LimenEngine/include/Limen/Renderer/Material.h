//
// Created by chenlong on 2026/8/28.
//

#pragma once

#include <string>
#include <unordered_map>
#include <cstdint>
#include <variant>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>
#include <glm/mat4x4.hpp>
#include "Limen/Core/Core.h"


namespace Limen
{
    class GraphicsPipeline;
    class Texture;


    /**
     * @brief Material 可以保存的普通的Shader参数类型
     *
     * 例如：
     * float          -> 粗糙度、高光指数
     * int32_t        -> 开关或有符号整数
     * uint32_t       -> 无符号整数
     * glm::vec2      -> UV 缩放
     * glm::vec3      -> RGB 颜色
     * glm::vec4      -> RGBA 颜色
     * glm::mat4      -> 纹理变换等矩阵参数
     */
    using MaterialParameterValue = std::variant<
        float,
        int32_t,
        uint32_t,
        glm::vec2,
        glm::vec3,
        glm::vec4,
        glm::mat4
    >;

    /**
     * @brief 一个材质纹理如何绑定到Shader
     */
    struct MaterialTextureBinding
    {
        // 实际的纹理资源，可以由多个Material共享
        Ref<Texture> TextureRes;

        // 绑定纹理槽，例如 0 表示 Texture slot 0
        uint32_t Slot;
    };

    /**
     * @brief 描述一次绘制所使用的材质状态。
     *
     * Material负责保存：
     *
     * 1. GraphicsPipeline；
     * 2. 普通Shader参数；
     * 3. 纹理及其sampler槽位；
     * 4. 调试名称。
     *
     * Material不保存Mesh、Camera、Transform、Framebuffer或RenderPass，
     * 也不会自行发出绘制命令。
     */
    class LIMEN_API Material
    {
    public:
        /**
         * @brief 创建一个通用材质
         *
         * @param pipeline 这个材质所使用的管线，Pipeline内部已经持有Shader
         * @param debugName 用于日志、编辑器和调试器显示的名称
         */
        explicit Material(
            Ref<GraphicsPipeline> pipeline,
            std::string debugName = "Unnamed Material"
        );

        /**
        * @brief 设置一个float材质参数。
        *
        * SetFloat("u_Shininess", 64.0f);
        */
        void SetFloat(
            const std::string &name,
            float value
        ) ;

        void SetInt(
            const std::string &name,
            int32_t value
        ) ;

        void SetUInt(
            const std::string &name,
            uint32_t value
        ) ;

        void SetFloat2(
            const std::string &name,
            const glm::vec2 &value
        ) ;

        void SetFloat3(
            const std::string &name,
            const glm::vec3 &value
        ) ;

        void SetFloat4(
            const std::string &name,
            const glm::vec4 &value
        ) ;

        void SetMat4(
            const std::string &name,
            const glm::mat4 &value
        ) ;

        /**
         * @brief 设置一张材质纹理
         *
         * @param samplerName 上传到shader的sampler的名称
         *
         * @param texture 当前材质的纹理
         *
         * @param slot 当前纹理的插槽位置
         */
        void SetTexture(const std::string &samplerName,
                        const Ref<Texture> &texture,
                        uint32_t slot
        );


        /**
         * @brief 绑定材质的Pipeline、参数和纹理。
         *
         * 调用后GPU会得到：
         *
         * 1. 当前Pipeline和Shader；
         * 2. 所有普通Shader参数；
         * 3. 所有纹理和sampler槽位。
         *
         * Bind只准备材质状态，不发出DrawIndexed。
         */
        void Bind() const;

        /**
         * @brief 获取材质使用的Pipeline。
         *
         * Renderer需要读取它的PrimitiveTopology和Shader。
         */
        [[nodiscard]]
        const GraphicsPipeline &GetPipeline() const;

        /**
         * @brief 获取材质的调试名称。
         */
        [[nodiscard]]
        const std::string &GetDebugName() const noexcept;

    private:
        // 多个Pipeline可以被多个材质共享
        Ref<GraphicsPipeline> m_Pipeline;

        // 参数名称  ->  参数值
        //Material 在 CPU 端保存的“普通 Shader 参数表”：
        std::unordered_map<std::string, MaterialParameterValue> m_Parameters;

        // Shader sampler名称 -> 纹理绑定信息
        //Material 在 CPU 端保存的 Texture 表
        std::unordered_map<std::string, MaterialTextureBinding> m_TexturesBindings;

        // Debug Name
        std::string m_DebugName;
    };
}
