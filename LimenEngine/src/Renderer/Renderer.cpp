//
// Created by chenlong on 2026/8/11.
//
#include "Limen/Renderer/Renderer2D.h"
#include "Limen/Renderer/Renderer.h"

#include "Limen/Core/Log.h"
#include "Limen/Renderer/Material.h"
#include "Limen/Renderer/Mesh.h"
#include "Limen/Renderer/Camera.h"
#include "Limen/Scene/Light.h"

namespace Limen
{
    namespace
    {
        /**
         * Renderer第一版能够上传给普通Uniform数组的最大点光源数量。
         *
         * 必须与BlinnPhong.frag中的LIMEN_MAX_POINT_LIGHTS保持一致。
         * 后续建立Shader配置系统后，再消除这份重复定义。
         */
        constexpr uint32_t MaxPointLightCount = 4;

        struct SceneData
        {
            bool IsActive = false;
            glm::mat4 ViewProjection{1.0f};
            glm::vec3 CameraPosition{0.0f};
            DirectionalLight MainDirectionalLight;
            /**
             * @brief 当前活动场景中的全部点光源快照。
             *
             * BeginScene时从Scene复制进来，
             * 后续所有Submit读取同一份光源数据。
             */
            std::vector<PointLight> PointLights;
        };

        SceneData s_SceneData;
    }

    void Renderer::BeginScene(const Camera &camera)
    {
        // 2D 和旧路径暂时不提供光源 因此使用 DirectionalLight
        BeginScene(camera, DirectionalLight{}, {});
    }

    void Renderer::BeginScene(const Camera &camera, const DirectionalLight &directionalLight)
    {
        BeginScene(camera, directionalLight, {});
    }

    void Renderer::BeginScene(const Camera &camera, const DirectionalLight &directionalLight,
                              const std::vector<PointLight> &pointLights)
    {
        if (s_SceneData.IsActive)
        {
            LM_CORE_ERROR("Another scene is already active");
            return;
        }

        /*
         * 在BeginScene时建立当前场景的帧内快照。
         *
         * 后续无论提交多少个Mesh，
         * 每次Submit读取的都是同一套相机和光源数据。
         */
        s_SceneData.CameraPosition = camera.GetPosition();
        s_SceneData.ViewProjection = camera.GetViewProjectionMatrix();
        s_SceneData.MainDirectionalLight = directionalLight;

        /*
         * 这里有意复制vector。
         *
         * BeginScene参数使用const引用，避免传参时复制；
         * Renderer再主动复制一份，保证BeginScene到EndScene期间
         * 使用的光源数据不会被Scene外部修改。
         */
        s_SceneData.PointLights = pointLights;

        s_SceneData.IsActive = true;
    }

    void Renderer::OnWindowResize(const uint32_t width, const uint32_t height)
    {
        if (width == 0 || height == 0)
            return;

        RendererCommand::SetViewport(0, 0, width, height);
    }

    void Renderer::EndScene()
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ASSERT(false, "Renderer::EndScene called without a matching BeginScene");
            return;
        }

        // Present 属于窗口帧生命周期；EndScene() 只关闭逻辑提交区间。
        s_SceneData.IsActive = false;
    }

    void Renderer::Init()
    {
        /*
         * 先创建并初始化底层 RendererAPI。
         *
         * Renderer2D 创建 VAO、VBO、Shader、UBO 和 Pipeline 时，
         * 会通过 RendererCommand 和 RHI 使用当前图形 API。
         */
        RendererCommand::Init();

        /*
         * 底层渲染后端和 GraphicsContext 已经可用，
         * 现在可以创建 Renderer2D 的公共 GPU 资源。
         */
        Renderer2D::Init();
    }

    void Renderer::Shutdown()
    {
        /*
         * 先释放依赖底层图形 API 的二维 GPU 资源。
         */
        Renderer2D::Shutdown();

        /*
         * 所有高层渲染资源释放后，再销毁底层 RendererAPI。
         */
        RendererCommand::Shutdown();
    }

    // 兼容尚未迁移到 GraphicsPipeline 的 2D 示例。
    void Renderer::Submit(
        const Ref<Shader> &shader,
        const VertexArray &vertexArray,
        const glm::mat4 &transform
    )
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ASSERT(false, "Renderer::Submit must be called between BeginScene and EndScene");
            return;
        }

        if (!shader)
        {
            LM_CORE_ASSERT(false, "Renderer::Submit received a null shader");
            return;
        }

        shader->Bind();

        shader->SetMat4(
            "u_ViewProjection",
            s_SceneData.ViewProjection
        );

        shader->SetMat4(
            "u_Transform",
            transform
        );

        shader->SetFloat3(
            "u_CameraPosition",
            s_SceneData.CameraPosition
        );

        vertexArray.Bind();
        RendererCommand::DrawIndexed(vertexArray);
    }

    void Renderer::Submit(
        const GraphicsPipeline &pipeline,
        const VertexArray &vertexArray,
        const glm::mat4 &transform
    )
    {
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ERROR("Renderer::Submit must be called between BeginScene and EndScene");
            return;
        }

        const GraphicsPipelineSpecification &specification = pipeline.GetSpecification();

        if (!specification.ShaderProgram)
        {
            LM_CORE_ERROR("Renderer::Submit received a pipeline without a shader");
            return;
        }

        pipeline.Bind();

        const Ref<Shader> &shader = specification.ShaderProgram;

        shader->SetMat4(
            "u_ViewProjection",
            s_SceneData.ViewProjection
        );

        shader->SetMat4(
            "u_Transform",
            transform
        );

        shader->SetFloat3(
            "u_CameraPosition",
            s_SceneData.CameraPosition
        );


        vertexArray.Bind();
        RendererCommand::DrawIndexed(
            vertexArray,
            specification.Topology
        );
    }

    void Renderer::Submit(const Material &material, const Mesh &mesh, const glm::mat4 &transform)
    {
        // 绘制只能发生在BeginScene和EndScene之间。
        if (!s_SceneData.IsActive)
        {
            LM_CORE_ERROR(
                "Renderer::Submit(Material, Mesh) must be called "
                "between BeginScene and EndScene"
            );
            return;
        }

        /**
         * Material::Bind() 负责：
         *
         * 1. 绑定pipeline和Shader
         * 2. 上传材质的普通参数
         * 3. 绑定纹理并设置sampler槽位
         */
        material.Bind();

        const GraphicsPipeline &pipeline = material.GetPipeline();
        const auto &specification = pipeline.GetSpecification();
        const auto &shader = specification.ShaderProgram;
        if (!shader)
        {
            LM_CORE_ERROR(
                "Material '{}' uses a pipeline without Shader",
                material.GetDebugName()
            );
            return;
        }

        /*
        * 这些参数属于Renderer，而不属于Material：
        *
        * ViewProjection：当前场景相机；
        * Transform：当前绘制物体；
        * CameraPosition：当前场景相机位置。
        *
        * 放在Material::Bind()之后上传，可以保证Renderer拥有的
        * 每帧、每物体参数不会被材质参数意外覆盖。
        */
        shader->SetMat4(
            "u_ViewProjection",
            s_SceneData.ViewProjection
        );

        shader->SetMat4(
            "u_Transform",
            transform
        );

        shader->SetFloat3(
            "u_CameraPosition",
            s_SceneData.CameraPosition
        );

        /*
         * 平行光属于场景数据，不属于某个 Material 或 Mesh。
         *
         * Material::Bind() 已经绑定了正确的 Shader，
         * 因此现在可以把本帧缓存的光源参数写入该 Shader。
         */
        const DirectionalLight &directionalLight = s_SceneData.MainDirectionalLight;

        // 光线从光源射向场景的方向。
        shader->SetFloat3(
            "u_DirectionalLightDirection",
            directionalLight.Direction
        );

        // 光源的线性 RGB 颜色。
        shader->SetFloat3(
            "u_DirectionalLightColor",
            directionalLight.Color
        );

        // 独立的亮度倍率。
        shader->SetFloat(
            "u_DirectionalLightIntensity",
            directionalLight.Intensity
        );

        //GLSL 数组容量固定为4 Scene 可以保存更多点光源，但当前钱箱渲染路径只把前四个上传给Shader
        const uint32_t pointLightCount = static_cast<uint32_t>(std::min<std::size_t>(
            s_SceneData.PointLights.size(), MaxPointLightCount));

        // Tell to shader what size of point lights we should deal
        shader->SetInt("u_PointLightCount", pointLightCount);

        /*
         * 按GLSL结构体数组的成员名称，
         * 逐个上传点光源数据。
         */
        for (uint32_t pointLightIndex = 0; pointLightIndex < pointLightCount; ++pointLightIndex)
        {
            const auto &[Position, Color, Intensity] = s_SceneData.PointLights[pointLightIndex];

            /*
             * pointLightIndex为0时，prefix为：
             * u_PointLights[0].
             */
            const std::string uniformPrefix =
                    "u_PointLights[" + std::to_string(pointLightIndex) + "].";

            shader->SetFloat3(
                (uniformPrefix + "Position").c_str(),
                Position
            );

            shader->SetFloat3(
                (uniformPrefix + "Color").c_str(),
                Color
            );

            shader->SetFloat(
                (uniformPrefix + "Intensity").c_str(),
                Intensity
            );
        }

        //Mesh使用本次绘制所用的VAO VBO IBO
        const VertexArray &vao = mesh.GetVertexArray();
        vao.Bind();

        RendererCommand::DrawIndexed(vao, specification.Topology);
    }
}
