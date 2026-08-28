//
// Created by chenlong on 2026/8/20.
//

#include "Example3DLayer.h"

#include <glm/ext/matrix_transform.hpp>
#include <glm/glm.hpp>

#include "imgui.h"
#include "Limen/Core/Log.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/Renderer.h"

namespace SandBox
{
    Example3DLayer::Example3DLayer()
        : Layer("3D Layer"),
          m_CameraController(
              45.f, 1600.f / 900.f, 0.1f, 100.f
          )
    {
        /**
         * @brief 立方体顶点数据。
         *
         * 每条顶点记录包含：
         *
         * Position：模型局部坐标，3个float；
         * Normal：模型局部空间法线，3个float；
         * TexCoord：二维纹理坐标，2个float，范围通常为[0, 1]。
         *
         * 每条顶点记录总共8个float。
         *
         * 立方体虽然只有8个不同的位置，但需要24条顶点记录。
         * 因为同一个角在三个面上具有不同的法线和UV。
         */
        constexpr float cubeVertices[] = {
            // Position                  // Normal             // TexCoord

            // 前面：z = +0.5
            -0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,

            // 后面：z = -0.5
            0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 1.0f, 1.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 0.0f, -1.0f, 0.0f, 1.0f,

            // 左面：x = -0.5
            -0.5f, -0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            -0.5f, -0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            -0.5f, 0.5f, 0.5f, -1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            // 右面：x = +0.5
            0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 1.0f,
            0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 0.0f, 1.0f,

            // 上面：y = +0.5
            -0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f,
            0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

            // 下面：y = -0.5
            -0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, -0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 0.0f,
            0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 1.0f, 1.0f,
            -0.5f, -0.5f, 0.5f, 0.0f, -1.0f, 0.0f, 0.0f, 1.0f
        };

        /**
         * 立方体索引：6 个面 × 每面 2 个三角形 × 每个三角形 3 个索引
         * = 36 个索引。每个面的绕序均与该面的外法线一致。
         */
        constexpr uint32_t cubeIndices[] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };

        Limen::MeshData cubeData;

        for (uint32_t i = 0; i < sizeof(cubeVertices) / sizeof(float); i += 8)
        {
            cubeData.Vertices.push_back(
                //C++20 标准增加了部分“聚合类型圆括号初始化”能力，但不同工具链及模板构造场景的支持并不完全一致。
                //所以不能写Limen::MeshVertex(xxxxx)
                Limen::MeshVertex{
                    .Position = glm::vec3{cubeVertices[i], cubeVertices[i + 1], cubeVertices[i + 2]},
                    .Normal = glm::vec3{cubeVertices[i + 3], cubeVertices[i + 4], cubeVertices[i + 5]},
                    .TexCoord = glm::vec2{cubeVertices[i + 6], cubeVertices[i + 7]}
                }
            );
        }

        for (uint32_t i = 0; i < sizeof(cubeIndices) / sizeof(uint32_t); ++i)
            cubeData.Indices.emplace_back(cubeIndices[i]);


        m_CubeMesh.reset(new Limen::Mesh(cubeData));

        Limen::FramebufferSpecification spec;
        spec.Width = 1280;
        spec.Height = 720;
        spec.Samples = 4;

        m_SceneFramebuffer = Limen::Framebuffer::Create(spec);
        LM_CORE_ASSERT(m_SceneFramebuffer, "Failed to create 3D scene Framebuffer");

        Limen::RenderPassSpecification sceneRenderPassSpec;
        // RenderPass 只借用 Framebuffer，不转移 unique_ptr 的所有权。
        sceneRenderPassSpec.TargetFramebuffer = m_SceneFramebuffer.get();
        sceneRenderPassSpec.ClearColor = {0.1f, 0.1f, 0.1f, 1.0f};
        sceneRenderPassSpec.DebugName = "Example3D Render Pass";

        m_SceneRenderPass = Limen::CreateScope<Limen::RenderPass>(sceneRenderPassSpec);



        /**
         * @brief 按当前RendererAPI加载3D Blinn-Phong Shader。
         *
         * 这里只提供后端无关的逻辑路径。ShaderLibrary会在OpenGL下
         * 自动选择OpenGL/Example3D/BlinnPhong.vert和.frag；以后选择
         * Direct3D 12时会改为DirectX12目录中的.vs.hlsl和.ps.hlsl。
         */
        m_CubeShader = m_ShaderLib->Load(
            "Example3D/BlinnPhong"
        );

        LM_CORE_ASSERT(
            m_CubeShader,
            "Failed to create Example3D BlinnPhong shader"
        );

        //region FBO config
        Limen::GraphicsPipelineSpecification cubePipelineSpec;

        cubePipelineSpec.ShaderProgram = m_CubeShader;
        cubePipelineSpec.Topology =
                Limen::PrimitiveTopology::TriangleList;

        cubePipelineSpec.DepthTestEnabled =
                true;

        cubePipelineSpec.DepthWriteEnabled =
                true;

        cubePipelineSpec.DepthCompare =
                Limen::CompareOperation::Less;

        cubePipelineSpec.Blend =
                Limen::BlendMode::Opaque;

        cubePipelineSpec.Culling =
                Limen::CullMode::Back;

        cubePipelineSpec.FrontFaceWinding =
                Limen::FrontFace::CounterClockwise;

        cubePipelineSpec.DebugName =
                "Example3D Cube Pipeline";

        m_CubePipeline = Limen::GraphicsPipeline::Create(
            cubePipelineSpec
        );

        LM_CORE_ASSERT(
            m_CubePipeline,
            "Failed to create Example3D cube graphics pipeline"
        );
        //endregion


        /**
         * @brief 加载立方体的Albedo纹理。
         *
         * 参数是相对于程序运行目录的资源路径。
         * CMake会把LimenSandBox/assets复制到可执行文件目录，
         * 因此运行时可以通过assets/textures/...访问。
         */
        m_AlbedoTexture = Limen::Texture2D::Create(
            "assets/textures/checkerboard.png"
        );

        LM_CORE_ASSERT(m_AlbedoTexture, "Failed to create cube albedo texture");

        m_CameraController.SetMouseLookEnabled(false);
    }

    void Example3DLayer::OnUpdate(Limen::DeltaTime &deltaTime)
    {
        if (m_ViewportWidth > 0 && m_ViewportHeight > 0 && (
                m_SceneFramebuffer->GetSpecification().Width != m_ViewportWidth ||
                m_SceneFramebuffer->GetSpecification().Height != m_ViewportHeight))
        {
            // 重新创建颜色、深度和 MSAA 附件。
            m_SceneFramebuffer->Resize(
                m_ViewportWidth,
                m_ViewportHeight
            );
            // 更新透视投影矩阵的宽高比。
            m_CameraController.OnResize(
                static_cast<float>(m_ViewportWidth),
                static_cast<float>(m_ViewportHeight)
            );
        }

        // 松开右键，结束本次导航。
        if (const bool rightMousePressed = Limen::Input::IsMouseButtonPressed(Limen::MouseButton::Right); !
            rightMousePressed)
        {
            m_ViewportNavigationActive = false;
        }
        // 右键必须从 Scene 面板内部按下，才能开始导航。
        else if (!m_ViewportNavigationActive &&
                 m_ViewportHovered)
        {
            m_ViewportNavigationActive = true;
        }

        m_CameraController.SetMouseLookEnabled(m_ViewportNavigationActive);
        // 必须先更新相机，再让BeginScene复制本帧的ViewProjection。
        m_CameraController.OnUpdate(deltaTime);

        // 绑定场景 Framebuffer，并清理本帧的颜色与深度附件。
        /**
         * 我们必选先从这开始
         * 这里会bind FBO 我们的GPU需要知道
         */
        m_SceneRenderPass->Begin();

        /**
         * 使用透视相机开始3D场景。
         *
         * BeginScene会复制相机的ViewProjection矩阵和位置。
         */
        Limen::Renderer::BeginScene(m_CameraController.GetCamera());

        /**
         * 创建立方体Model矩阵。
         *
         * 第一个参数：原始Model矩阵；
         * 第二个参数：旋转角度，GLM要求弧度；
         * 第三个参数：旋转轴，这里使用世界/局部Y轴。
         */
        glm::mat4 cubeTransform{1.0f};

        // 绕世界/当前Y轴旋转。
        cubeTransform = glm::rotate(
            cubeTransform,
            glm::radians(m_CubeRotationDegrees),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        // 再绕X轴旋转一半的角度，便于观察上表面。
        cubeTransform = glm::rotate(
            cubeTransform,
            glm::radians(m_CubeRotationDegrees * 0.5f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        // Shader 中的 u_AlbedoTexture 约定从纹理槽 0 采样。
        m_AlbedoTexture->Bind(0);
        // Limen::Renderer::Submit(
        //     *m_CubePipeline,
        //     *m_CubeVAO, //记住了Albedo_Texture
        //     cubeTransform
        // );

        Limen::Renderer::Submit(
            *m_CubePipeline,
            m_CubeMesh->GetVertexArray(), //记住了Albedo_Texture
            cubeTransform
        );
        Limen::Renderer::EndScene();

        // 解绑场景 Framebuffer，并把 MSAA 颜色解析到可采样的 2D 纹理。
        m_SceneRenderPass->End();
    }

    void Example3DLayer::OnEvent(Limen::Event &event)
    {
        m_CameraController.OnEvent(event);
    }

    void Example3DLayer::OnImGuiRender()
    {
        const bool sceneVisible = ImGui::Begin("Scene");

        m_ViewportFocused = sceneVisible && ImGui::IsWindowFocused();

        m_ViewportHovered = sceneVisible && ImGui::IsWindowHovered();

        if (sceneVisible)
        {
            if (const ImVec2 viewportSize = ImGui::GetContentRegionAvail();
                viewportSize.x > 0.0f && viewportSize.y > 0.0f)
            {
                /**
                 * 此处只记录 ImGui 内容区尺寸；下一帧 OnUpdate() 在绘制前
                 * Resize Framebuffer，避免先显示旧尺寸纹理再重新分配附件。
                 */
                m_ViewportWidth = static_cast<uint32_t>(viewportSize.x);

                m_ViewportHeight = static_cast<uint32_t>(viewportSize.y);

                const ImTextureID textureID = m_SceneFramebuffer->GetColorAttachmentHandle();

                ImGui::Image(ImTextureRef(textureID),
                             viewportSize,
                             // OpenGL 纹理原点与 ImGui 图像坐标原点相反，因此翻转 V。
                             ImVec2(0.0f, 1.0f),
                             ImVec2(1.0f, 0.0f)
                );
            }
        }
        ImGui::End();
    }
} // SandBox
