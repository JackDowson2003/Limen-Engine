//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include  <vector>
#include "Limen/Core/Core.h"
#include "Limen/Renderer/RendererCommand.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Shader.h"

#include "Limen/Scene/Light.h"

namespace Limen
{
    class Camera;
    class Material;
    class Mesh;

    class LIMEN_API Renderer
    {
    public:
        /**
         * @brief 开始逻辑场景提交区间，并缓存本帧相机数据。
         *
         * BeginScene() 与 EndScene() 必须成对调用；同一时刻只能有一个活动场景。
         */
        static void BeginScene(const Camera &camera);

        /**
         * @brief 使用指定相机和主平行光开始渲染场景。
         *
         * @param camera 本次场景使用的相机。
         * @param directionalLight 本次场景使用的主平行光。
         */
        static void BeginScene(
            const Camera &camera,
            const DirectionalLight &directionalLight
        );

        /**
         * @brief 使用相机、主平行光和多个点光源开始场景。
         *
         * @param camera
         * 当前场景使用的相机，提供ViewProjection和相机世界坐标。
         *
         * @param directionalLight
         * 当前场景的主平行光。
         *
         * @param pointLights
         * 当前场景中的全部点光源。
         * 使用const引用，避免在函数传参阶段复制整个vector。
         */
        static void BeginScene(
            const Camera& camera,
            const DirectionalLight& directionalLight,
            const std::vector<PointLight>& pointLights
        );

        /**
         * @brief 使用完整的GAMES101场景光照数据开始场景。
         *
         * @param camera
         * 当前场景使用的相机。
         *
         * @param ambientLight
         * 当前场景的常量环境光I_a。
         *
         * @param directionalLight
         * 当前场景的主平行光。
         *
         * @param pointLights
         * 当前场景中的全部点光源。
         */
        static void BeginScene(
            const Camera& camera,
            const AmbientLight& ambientLight,
            const DirectionalLight& directionalLight,
            const std::vector<PointLight>& pointLights
        );

        /**
         * @brief 使用完整光照和阴影数据开始3D场景。
         *
         * @param directionalLightViewProjection
         * 世界空间到平行光裁剪空间的矩阵。
         *
         * @param shadowMapTextureSlot
         * Shadow Map当前绑定的纹理槽。
         */
        static void BeginScene(
            const Camera& camera,
            const AmbientLight& ambientLight,
            const DirectionalLight& directionalLight,
            const std::vector<PointLight>& pointLights,
            const glm::mat4& directionalLightViewProjection,
            uint32_t shadowMapTextureSlot
        );

        /**
         * @brief 更新Renderer输出使用的GPU Viewport。
         *
         * 该函数不修改Camera的投影矩阵；CameraController会独立处理宽高比。
         */
        static void OnWindowResize(uint32_t width, uint32_t height);

        static void EndScene();

        static void Init();

        /**
         * @brief 释放Renderer持有的后端对象。
         *
         * 调用时OpenGL Context仍然必须有效。
         */
        static void Shutdown();

        /**
         * @brief 旧的 Shader 直接提交入口，保留到 2D 示例迁移完成。
         *
         * 新代码应优先使用 GraphicsPipeline 重载，由 Pipeline 统一管理固定状态。
         */
        static void Submit(
            const Ref<Shader> &shader,
            const VertexArray &vertexArray,
            const glm::mat4 &transform = glm::mat4(1.0f)
        );

        /**
         * @brief 使用完整 GraphicsPipeline 提交一次索引绘制。
         *
         * @param pipeline
         * 本次绘制使用的 Shader 和固定功能状态。
         *
         * @param vertexArray
         * 本次绘制使用的顶点和索引数据。
         *
         * @param transform
         * 当前物体从模型空间变换到世界空间的矩阵。
         *
         * 调用顺序为：绑定 Pipeline、上传绘制参数、绑定几何数据、发出绘制命令。
         */
        static void Submit(
            const GraphicsPipeline &pipeline,
            const VertexArray &vertexArray,
            const glm::mat4 &transform = glm::mat4(1.0f)
        );

        /**
         * @brief 使用材质和Mesh提交一次3D索引绘制。
         *
         * Material提供Pipeline、Shader参数和纹理；
         * Mesh提供VAO、VBO和IBO所表示的几何数据；
         * Renderer负责上传相机与物体数据并发出绘制命令。
         *
         * @param material 本次绘制使用的材质。
         * @param mesh 本次绘制使用的几何数据。
         * @param transform Mesh从模型空间变换到世界空间的矩阵。
         */
        static void Submit(
            const Material &material,
            const Mesh &mesh,
            const glm::mat4 &transform = glm::mat4(1.0f)
        );

        /**
         * @brief 提交一次只写深度的几何绘制。
         *
         *  普通 Submit：
            Material + Mesh + Camera + Lighting → 输出颜色和深度

            SubmitDepth：
            ShadowPipeline + Mesh + LightVP → 只输出深度
         *
         * 不使用Material，因为Shadow Pass只需要：
         * - 深度Pipeline；
         * - Mesh几何数据；
         * - 光源ViewProjection；
         * - 物体Transform。
         *
         * @param pipeline Shadow Depth Pipeline。
         * @param mesh 要绘制的Mesh。
         * @param lightViewProjection 世界空间到光源裁剪空间的矩阵。
         * @param transform 模型局部空间到世界空间的矩阵。
         */
        static void SubmitDepth(
            const GraphicsPipeline& pipeline,
            const Mesh& mesh,
            const glm::mat4& lightViewProjection,
            const glm::mat4& transform = glm::mat4(1.0f)
        );

        static RendererAPI::API GetRenderAPI()
        {
            return RendererAPI::GetAPI();
        }
    };
}
