//
// Created by chenlong on 2026/8/20.
//

#include <cmath>

#include <glm/ext/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "Example3DLayer.h"
#include "imgui.h"
#include "Limen/Core/Log.h"
#include "Limen/Input/Input.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Texture.h"

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


        m_CubeMesh = Limen::CreateRef<Limen::Mesh>(cubeData);

        // 创建负责当前 Scene Viewport 的场景渲染器。
        Limen::SceneRendererSpecification sceneSpec;

        // RenderPass 只借用 Framebuffer，不转移 unique_ptr 的所有权。
        sceneSpec.Width = m_ViewportWidth;
        sceneSpec.Height = m_ViewportHeight;
        sceneSpec.Samples = 4;
        sceneSpec.ClearColor = glm::vec4(0.1f, 0.1f, 0.1f, 1.0f);
        sceneSpec.DebugName = "Example3D Scene Renderer";
        m_SceneRenderer = Limen::CreateScope<Limen::SceneRenderer>(sceneSpec);

        /**
         * @brief 按当前RendererAPI加载3D Blinn-Phong Shader。
         *
         * 这里只提供后端无关的逻辑路径。ShaderLibrary会在OpenGL下
         * 自动选择OpenGL/Example3D/BlinnPhong.vert和.frag；以后选择
         * Direct3D 12时会改为DirectX12目录中的.vs.hlsl和.ps.hlsl。
         */
        const Limen::Ref<Limen::Shader> shader =
                m_ShaderLib->Load(
                    "Example3D/BlinnPhong"
                );
        LM_CORE_ASSERT(
            shader,
            "Failed to create Example3D BlinnPhong shader"
        );

        //region Pipeline config
        Limen::GraphicsPipelineSpecification cubePipelineSpec;

        cubePipelineSpec.ShaderProgram = shader;
        cubePipelineSpec.Topology = Limen::PrimitiveTopology::TriangleList;
        cubePipelineSpec.DepthTestEnabled = true;
        cubePipelineSpec.DepthWriteEnabled = true;
        cubePipelineSpec.DepthCompare = Limen::CompareOperation::Less;
        cubePipelineSpec.Blend = Limen::BlendMode::Opaque;
        cubePipelineSpec.Culling = Limen::CullMode::Back;
        cubePipelineSpec.FrontFaceWinding = Limen::FrontFace::CounterClockwise;
        cubePipelineSpec.DebugName = "Example3D Cube Pipeline";
        //endregion

        const Limen::Ref<Limen::GraphicsPipeline> pipeline = Limen::GraphicsPipeline::Create(
            cubePipelineSpec
        );

        LM_CORE_ASSERT(
            pipeline,
            "Failed to create Example3D cube graphics pipeline"
        );


        /**
         * @brief 加载立方体的Albedo纹理。
         *
         * 参数是相对于程序运行目录的资源路径。
         * CMake会把LimenSandBox/assets复制到可执行文件目录，
         * 因此运行时可以通过assets/textures/...访问。
         */
        const Limen::Ref<Limen::Texture2D> texture = Limen::Texture2D::Create(
            "assets/textures/checkerboard.png"
        );

        LM_CORE_ASSERT(texture, "Failed to create cube albedo texture");

        //使用立方体Pipeline的材质
        m_CubeMaterial = Limen::CreateRef<Limen::Material>(
            pipeline,
            "Example3D Cube Material"
        );

        LM_CORE_ASSERT(
            m_CubeMaterial,
            "Failed to create Example3D cube material"
        );

        if (m_CubeMaterial)
        {
            m_CubeMaterial->SetTexture(
                "u_AlbedoTexture",
                texture,
                0
            );

            // 设置Blinn-Phong高光指数p。
            m_CubeMaterial->SetFloat(
                "u_Shininess",
                128.f
            );

            // 设置GAMES101中的材质镜面反射系数k_s。
            m_CubeMaterial->SetFloat3(
                "u_SpecularColor",
                glm::vec3(0.35f)
            );
        }

        //初始时不允许鼠标控制
        m_CameraController.SetMouseLookEnabled(false);

        /*
         * 显式设置当前 Scene 的主平行光。
         *
         * 使用偏冷的蓝色，方便确认光源数据确实经过：
         * Scene → SceneRenderer → Renderer → Shader。
         */
        Limen::DirectionalLight mainDirectionalLight;

        // 光从右上前方射向场景。
        mainDirectionalLight.Direction =
                glm::vec3(1.0f, -1.0f, -1.0f);

        // 偏冷色的线性 RGB。
        mainDirectionalLight.Color =
                glm::vec3(0.35f, 0.55f, 1.0f);

        // 当前测试使用普通亮度倍率。
        mainDirectionalLight.Intensity = 1.0f;

        m_Scene.SetDirectionalLight(mainDirectionalLight);

        /*
         * 创建用于测试GAMES101 Blinn-Phong光照的点光源。
         *
         * 目前只是把光源加入Scene；
         * Renderer和Shader还没有读取它，所以暂时不会改变画面。
         */
        Limen::PointLight pointLight;

        // 点光源位于两个立方体的右上前方
        pointLight.Position = glm::vec3(2.0f, 2.0f, 2.0f);

        // 使用白色光，方便观察材质本身的颜色
        pointLight.Color = glm::vec3(1.0f);

        // 后续使用平方衰减
        pointLight.Intensity = 10.0f;

        m_Scene.AddPointLight(pointLight);

        /*
         * SceneRenderObject 把资源和物体的世界变换组合起来。
         *
         * Mesh 和 Material 使用 Ref，共享的是同一个资源对象，
         * 不会复制底层 VAO、VBO、IBO、Shader 或纹理。
         */
        Limen::SceneRenderObject cubeObject;

        cubeObject.MeshResource = m_CubeMesh;
        cubeObject.MaterialResource = m_CubeMaterial;
        cubeObject.Transform = glm::mat4(1.0f);

        m_CubeObjectHandle = m_Scene.AddRenderObject(cubeObject);

        LM_CORE_ASSERT(
            m_CubeObjectHandle.IsValid(),
            "Failed to add cube object to Scene"
        );

        /*
         * 第二个立方体共享第一份 Mesh 和 Material。
         *
         * 这里只创建新的 SceneRenderObject，
         * 不会重新创建 VAO、VBO、IBO、Shader 或纹理。
         */
        Limen::SceneRenderObject secondCubeObject;

        secondCubeObject.MeshResource = m_CubeMesh;
        secondCubeObject.MaterialResource = m_CubeMaterial;
        secondCubeObject.Transform = glm::translate(glm::mat4(1.0f),
                                                    glm::vec3(1.5f, 0.0f, .0f));

        /*
         * 第二个立方体当前不需要每帧修改，
         * 所以句柄只用于检查添加是否成功，不必保存为成员。
         */
        const Limen::SceneRenderObjectHandle secondCubeHandle =
                m_Scene.AddRenderObject(secondCubeObject);

        LM_CORE_ASSERT(
            secondCubeHandle.IsValid(),
            "Failed to add second cube object to Scene"
        );
    }

    /**
     *
     * 同步 Viewport 尺寸
            ↓
        处理相机输入
            ↓
        更新相机
            ↓
        把 Scene 和 Camera 交给 SceneRenderer
     * @param deltaTime deltaTime
     */
    void Example3DLayer::OnUpdate(Limen::DeltaTime &deltaTime)
    {
        LM_CORE_ASSERT(m_SceneRenderer, "Scene renderer is not initialized");

        if (!m_SceneRenderer)
            return;

        /*
         * ImGui Scene 面板尺寸改变时，需要同步更新两个不同的对象：
         *
         * SceneRenderer：修改 FBO 颜色、深度和 MSAA 附件尺寸；
         * CameraController：修改透视投影矩阵的宽高比。
         */
        if (const auto &sceneRendererSpec = m_SceneRenderer->GetSpecification();
            m_ViewportWidth > 0 && m_ViewportHeight > 0 &&
            (sceneRendererSpec.Width != m_ViewportWidth ||
             sceneRendererSpec.Height != m_ViewportHeight)
        )
        {
            m_SceneRenderer->Resize(
                m_ViewportWidth,
                m_ViewportHeight
            );

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

        /*
         * 每帧增加的角度 =
         * 每秒旋转角度 × 当前帧经过的秒数。
         *
         * 因此无论是 60 FPS 还是 120 FPS，
         * 一秒累计旋转的角度都是 m_CubeRotationSpeed。
         */
        m_CubeRotationDegrees += m_CubeRotationSpeed * deltaTime.GetSeconds();

        /*
         * 防止程序长时间运行后角度不断增大，
         * 造成浮点数精度逐渐下降。
         *
         * remainder 会把结果保持在大约 [-180, 180]。
         */
        m_CubeRotationDegrees = std::remainder(m_CubeRotationDegrees, 360.0f);

        // 从单位矩阵开始构造当前立方体的矩阵模型
        glm::mat4 cubeTransform{1.f};

        //围绕 Y 轴的旋转
        cubeTransform = glm::rotate(
            cubeTransform,
            glm::radians(m_CubeRotationDegrees),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

        //组合绕 X 轴的旋转，方便观察立方体上表面
        cubeTransform = glm::rotate(
            cubeTransform,
            glm::radians(m_CubeRotationDegrees * 0.5f),
            glm::vec3(1.0f, 0.0f, 0.0f)
        );

        /*
         * Layer 不直接访问 Scene 内部 vector，
         * 而是通过创建立方体时取得的句柄修改 Transform。
         */
        const bool transformUpdated = m_Scene.SetRenderObjectTransform(m_CubeObjectHandle, cubeTransform);

        LM_CORE_ASSERT(transformUpdated, "Transform update failed");

        if (!transformUpdated)
            return;

        /*
         * SceneRenderer 内部执行：
         *
         * RenderPass::Begin()
         * Renderer::BeginScene()
         * 遍历 SceneRenderObject 并 Submit()
         * Renderer::EndScene()
         * RenderPass::End()
         */
        m_SceneRenderer->Render(
            m_Scene,
            m_CameraController.GetCamera()
        );
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

                /*
                 * SceneRenderer::Render() 返回前已经结束 RenderPass，
                 * 因此 MSAA 颜色已经 Resolve 到可采样的单采样纹理。
                 */
                if (m_SceneRenderer)
                {
                    const std::uintptr_t colorAttachmentHandle =
                            m_SceneRenderer->GetFinalColorAttachmentHandle();

                    // 0 表示没有有效的纹理
                    if (colorAttachmentHandle != 0)
                    {
                        /*
                         * ImGui 的 OpenGL 后端把 ImTextureID 解释为纹理 ID。
                         * 这层转换只出现在客户端 ImGui 显示代码中，
                         * SceneRenderer 本身仍保持跨图形 API。
                         */
                        const ImTextureID textureID = colorAttachmentHandle;

                        ImGui::Image(ImTextureRef(textureID),
                                     viewportSize,
                                     // OpenGL 纹理坐标原点位于左下角，
                                     // ImGui 图像坐标原点位于左上角，因此翻转 V。
                                     ImVec2(0.0f, 1.0f),
                                     ImVec2(1.0f, 0.0f)
                        );
                    }
                }
            }
        }
        ImGui::End();

        /*
         * Lighting 面板与 Scene Viewport 分离，
         * 避免光源控件占用场景画面的显示区域。
         */
        const bool lightingVisible = ImGui::Begin("Lighting");

        if (lightingVisible)
        {
            /*
             * Getter 返回 const 引用，不能直接交给 ImGui 修改。
             * 因此先复制一份编辑中的光源数据，
             * 修改成功后再通过 Setter 写回 Scene。
             */
            Limen::DirectionalLight editableLight = m_Scene.GetDirectionalLight();
            bool lightChanged = false;

            /*
             * 使用 |= 而不是 ||：
             * 每个 ImGui 控件都必须执行并绘制，
             * 不能因为前一个控件返回 true 就短路后面的控件。
             */
            lightChanged |= ImGui::DragFloat3("Direction Light",
                                              glm::value_ptr(editableLight.Direction),
                                              0.05f, -1.f, 1.f
            );

            lightChanged |= ImGui::ColorEdit3(
                "Color",
                glm::value_ptr(editableLight.Color)
            );

            lightChanged |= ImGui::DragFloat(
                "Intensity",
                &editableLight.Intensity,
                0.05f,
                0.0f,
                10.0f,
                "%.2f"
            );

            const float directionLengthSquared =
                    glm::dot(
                        editableLight.Direction,
                        editableLight.Direction
                    );
            if (directionLengthSquared <= 0.000000001f)
            {
                ImGui::TextColored(
                    ImVec4(1.0f, 0.35f, 0.35f, 1.0f),
                    "Direction cannot be zero."
                );
            }
            else if (lightChanged)
            {
                m_Scene.SetDirectionalLight(editableLight);
            }
        }
        ImGui::End();
    }
} // SandBox
