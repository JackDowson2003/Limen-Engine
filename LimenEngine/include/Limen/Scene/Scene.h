//
// Created by chenlong on 2026/9/7.
//

#pragma once

#include <vector>
#include <limits>
#include <glm/mat4x4.hpp>

#include "Limen/Core/Core.h"
#include "Limen/Scene/Light.h"

namespace Limen
{
    class Mesh;
    class Material;

    /**
     * @brief Scene中一个可以被提交给Renderer的物体
     *
     * 当前第一版只保存渲染一个物体所需的最小数据
     *
     * 1. Mesh：物体的几何形状
     * 2. Material：物体的Shader、Pipeline、纹理以及材质参数
     * 3. Transform：物体从模型空间到世界空间的变换
     *
     * SceneRenderObject 只描述在场景中有什么，不会调用
     * Renderer、RendererPass或者任何图形底层API
     */
    struct SceneRenderObject
    {
        /**
         * @brief 物体所使用的几何资源
         *
         * 使用Ref是因为多个场景可以共享同一个Mesh
         * 例如一百个立方体可以共用同一份立方体顶点和索引数据
         */
        Ref<Mesh> MeshResource;

        /**
       * @brief 物体使用的材质资源。
       *
       * 多个物体可以共享同一个 Material；如果需要不同材质参数，
       * 后续可以创建不同的 Material 实例。
       */
        Ref<Material> MaterialResource;

        /**
         * @brief 物体从模型空间变换到世界空间的矩阵。
         *
         * 单位矩阵表示物体保持 Mesh 原始的位置、旋转和大小。
         */
        glm::mat4 Transform{1.0f};
    };

    /**
     * @brief 标识 Scene 中的一个可渲染对象。
     *
     * 当前第一版使用 vector 下标作为句柄。
     * 因为 Scene 暂时不支持删除和重新排序，所以对象加入后，
     * 它的下标在 Scene 生命周期内保持不变。
     *
     * 后续加入对象删除功能时，需要扩展为：
     * Index + Generation，避免旧句柄错误地访问新对象。
     */
    struct SceneRenderObjectHandle
    {
        /**
         * @brief 表示句柄没有指向任何有效对象。
         */
        static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

        /**
         * @brief 对象在 Scene::m_RenderObjects 中的下标。
         */
        uint32_t Index = InvalidIndex;

        /**
         * @brief 判断句柄是否至少不是默认的无效值。
         *
         * 这里只检查是否拥有下标；
         * Scene 后续还要检查 Index 是否越过 vector 边界。
         */
        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return Index != InvalidIndex;
        }
    };

    /**
     * @brief 标识Scene中的一个点光源
     *
     * 当前第一版使用m_PointLights中的vector下标作为句柄。
     * 由于暂时不支持删除和重新排序，点光源加入Scene后，
     * 对应下标在Scene生命周期内保持不变。
     *
     * 后续支持删除时，需要扩展为Index + Generation，
     * 避免旧句柄访问到后来占用同一位置的新光源。
     */
    struct ScenePointLightHandle
    {
        // 表示 Handle 没有只想有效点光源 初始为 四字节全1
        static constexpr uint32_t InvalidIndex = std::numeric_limits<uint32_t>::max();

        // 点光源在 Scene::m_PointLights中的下标
        uint32_t Index = InvalidIndex;

        /**
         * @brief 判断句柄是否至少拥有一个有效形式的下标。
         *
         * 这里只排除InvalidIndex；
         * Scene访问时还必须继续检查是否越过vector边界。
         */
        [[nodiscard]]
        bool IsValid() const noexcept
        {
            return Index != InvalidIndex;
        }
    };

    /**
     * @brief 保存一个场景中的对象数据
     *
     * Scene 只负责拥有和组织厂家数据，不负责发出任何渲染指令
     * SceneRenderer 后续会通过 GetRendererObjects() 读取这些对象并提交渲染
     */
    class LIMEN_API Scene final
    {
    public:
        Scene() = default;

        ~Scene() = default;

        /**
         * @brief 向 Scene 中加入一个可渲染对象。
         *
         * @param renderObject
         * 需要复制进 Scene 的 Mesh、Material 和 Transform。
         *
         * @return 新对象在当前 Scene 中的句柄；
         * 创建失败时返回无效句柄。
         */
        [[nodiscard]]
        SceneRenderObjectHandle AddRenderObject(
            const SceneRenderObject& renderObject
        );

        /**
         * @brief  设置当前场景的主平行光
         *
         * 第一版只支持一个主平行光
         *
         * @param light
         * 包含光线的传播方向、RGB颜色和强度
         */
        void SetDirectionalLight(const DirectionalLight& light);

        /**
         * @brief 设置当前场景的常量环境光。
         *
         * @param light
         * 环境光的RGB颜色和整体强度。
         */
        void SetAmbientLight(const AmbientLight& light);

        /**
         * @brief 获取当前场景的常量环境光。
         *
         * 返回const引用，避免复制并禁止外部绕过Scene直接修改。
         */
        [[nodiscard]]
        const AmbientLight&
        GetAmbientLight() const noexcept;

        /**
         * @brief 向场景中添加一个点光源。
         *
         * @param light
         * 点光源的位置、RGB颜色和强度。
         */
        [[nodiscard]]
        ScenePointLightHandle AddPointLight(const PointLight& light);

        /**
         * @brief 修改Scene中指定的点光源。
         *
         * @param handle
         * AddPointLight()返回的点光源句柄。
         * 句柄按值传递，因为它目前只包含一个uint32_t。
         *
         * @param light
         * 需要写入Scene的新点光源数据。
         *
         * @return
         * 修改成功返回true；
         * 句柄无效、越界或光源数据无效时返回false。
         */
        [[nodiscard]]
        bool SetPointLight(
            ScenePointLightHandle handle,
            const PointLight& light
        );

        /**
         * @brief 尝试读取Scene中指定的点光源。
         *
         * @param handle
         * AddPointLight()返回的点光源句柄。
         *
         * @param outLight
         * 读取成功时，将对应点光源的数据复制到这里。
         *
         * @return
         * 句柄有效且未越界时返回true；
         * 否则返回false，并且不修改outLight。
         */
        [[nodiscard]]
        bool TryGetPointLight(
            ScenePointLightHandle handle,
            PointLight& outLight
        ) const noexcept;

        /**
         * @brief 获取场景中的全部点光源。
         *
         * 返回const引用，避免复制整个数组，
         * 同时防止外部绕过Scene直接增删光源。
         */
        [[nodiscard]]
        const std::vector<PointLight>&
        GetPointLights() const noexcept;

        /**
         * @brief 修改指定场景对象的模型变换矩阵。
         *
         * @param handle
         * AddRenderObject() 返回的对象句柄。
         * 句柄按值传递，因为它目前只有一个 uint32_t。
         *
         * @param transform
         * 新的模型矩阵。使用 const 引用避免传参时复制整个 mat4；
         * Scene 最终会把它复制到对应对象的 Transform 中。
         *
         * @return 修改成功返回 true；
         * 句柄无效或越界时返回 false。
         */
        [[nodiscard]]
        bool SetRenderObjectTransform(
            SceneRenderObjectHandle handle,
            const glm::mat4& transform
        );

        /**
         * @brief 获取场景中所有可渲染对象。
         *
         * 返回 const 引用，不复制整个数组，同时避免 SceneRenderer
         * 在渲染过程中意外增删场景对象。
         */
        [[nodiscard]]
        const std::vector<SceneRenderObject> &
        GetRenderObjects() const noexcept;

        /**
         * @brief 获取当前场景的主平行光。
         *
         * 返回 const 引用，不复制光源数据，
         * 同时禁止外部绕过 Scene 直接修改。
         */
       [[nodiscard]]
       const DirectionalLight&
       GetDirectionalLight() const noexcept;

    private:
        /**
         * @brief 当前场景拥有的全部可渲染对象。
         *
         * vector 拥有 SceneRenderObject；其中的 Ref 共同持有对应的
         * Mesh 和 Material 资源。
         */
        std::vector<SceneRenderObject> m_RenderObjects;

        /**
         * @brief 当前场景的主平行光。
         *
         * DirectionalLight 是普通的小型数据，不拥有 GPU 资源，
         * 因此直接按值保存，不需要 Scope 或 Ref。
         */
        DirectionalLight m_DirectionalLight;

        /**
         * @brief 当前场景中的全部点光源。
         *
         * 每个点光源都会在着色时产生一份漫反射和镜面反射贡献，
         * 最终由Renderer累加。
         */
        std::vector<PointLight> m_PointLights;

        /**
         * @brief 当前场景的常量环境光。
         *
         * AmbientLight是普通CPU数据，不拥有GPU资源，
         * 因此直接按值保存。
         */
        AmbientLight m_AmbientLight;
    };
}
