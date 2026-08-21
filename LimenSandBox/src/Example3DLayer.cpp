//
// Created by chenlong on 2026/8/20.
//

#include "Example3DLayer.h"

#include "Limen/Renderer/Renderer.h"
#include "glm/glm.hpp"
#include <glm/ext/matrix_transform.hpp>

#include "Limen/Core/Log.h"


namespace SandBox
{
    Example3DLayer::Example3DLayer()
        : Layer("3D Layer"),
          m_Camera(
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
            -0.5f, -0.5f,  0.5f,        0.0f,  0.0f,  1.0f,   0.0f, 0.0f,
             0.5f, -0.5f,  0.5f,        0.0f,  0.0f,  1.0f,   1.0f, 0.0f,
             0.5f,  0.5f,  0.5f,        0.0f,  0.0f,  1.0f,   1.0f, 1.0f,
            -0.5f,  0.5f,  0.5f,        0.0f,  0.0f,  1.0f,   0.0f, 1.0f,

            // 后面：z = -0.5
             0.5f, -0.5f, -0.5f,        0.0f,  0.0f, -1.0f,   0.0f, 0.0f,
            -0.5f, -0.5f, -0.5f,        0.0f,  0.0f, -1.0f,   1.0f, 0.0f,
            -0.5f,  0.5f, -0.5f,        0.0f,  0.0f, -1.0f,   1.0f, 1.0f,
             0.5f,  0.5f, -0.5f,        0.0f,  0.0f, -1.0f,   0.0f, 1.0f,

            // 左面：x = -0.5
            -0.5f, -0.5f, -0.5f,       -1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
            -0.5f, -0.5f,  0.5f,       -1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
            -0.5f,  0.5f,  0.5f,       -1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,       -1.0f,  0.0f,  0.0f,   0.0f, 1.0f,

            // 右面：x = +0.5
             0.5f, -0.5f,  0.5f,        1.0f,  0.0f,  0.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,        1.0f,  0.0f,  0.0f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,        1.0f,  0.0f,  0.0f,   1.0f, 1.0f,
             0.5f,  0.5f,  0.5f,        1.0f,  0.0f,  0.0f,   0.0f, 1.0f,

            // 上面：y = +0.5
            -0.5f,  0.5f,  0.5f,        0.0f,  1.0f,  0.0f,   0.0f, 0.0f,
             0.5f,  0.5f,  0.5f,        0.0f,  1.0f,  0.0f,   1.0f, 0.0f,
             0.5f,  0.5f, -0.5f,        0.0f,  1.0f,  0.0f,   1.0f, 1.0f,
            -0.5f,  0.5f, -0.5f,        0.0f,  1.0f,  0.0f,   0.0f, 1.0f,

            // 下面：y = -0.5
            -0.5f, -0.5f, -0.5f,        0.0f, -1.0f,  0.0f,   0.0f, 0.0f,
             0.5f, -0.5f, -0.5f,        0.0f, -1.0f,  0.0f,   1.0f, 0.0f,
             0.5f, -0.5f,  0.5f,        0.0f, -1.0f,  0.0f,   1.0f, 1.0f,
            -0.5f, -0.5f,  0.5f,        0.0f, -1.0f,  0.0f,   0.0f, 1.0f
        };

        /**
        * 立方体索引。
        *
        * 6个面 × 每面2个三角形 × 每个三角形3个索引
        * = 36个索引。
        */
        constexpr uint32_t cubeIndices[] = {
            0, 1, 2, 2, 3, 0,
            4, 5, 6, 6, 7, 4,
            8, 9, 10, 10, 11, 8,
            12, 13, 14, 14, 15, 12,
            16, 17, 18, 18, 19, 16,
            20, 21, 22, 22, 23, 20
        };
        // 创建用于保存立方体顶点输入状态的VAO。
        m_CubeVAO.reset(Limen::VertexArray::Create());

        /**
         * 创建顶点缓冲。
         *
         * 第一个参数：CPU顶点数组的首地址。
         * 第二个参数：整个数组占用的字节数。
         */
        m_CubeVBO.reset(
            Limen::VertexBuffer::Create(
                cubeVertices,
                sizeof(cubeVertices)
            )
        );

        /**
         * @brief 描述一条立方体顶点记录的内存布局。
         *
         * location 0：Position，3个float；
         * location 1：Normal，3个float；
         * location 2：TexCoord，2个float。
         *
         * 总步长为：
         *
         *     (3 + 3 + 2) * sizeof(float) = 32字节
         */
        const Limen::BufferLayout cubeLayout
        {
            {
                Limen::ShaderDataType::Float3,
                "a_Position"
            },
            {
                Limen::ShaderDataType::Float3,
                "a_Normal"
            },
            {
                Limen::ShaderDataType::Float2,
                "a_TexCoord"
            }
        };

        m_CubeVBO->SetLayout(cubeLayout);
        m_CubeVAO->AddVertexBuffer(m_CubeVBO);

        /**
         * 创建索引缓冲。
         *
         * 第一个参数：索引数组首地址。
         * 第二个参数：索引数量，不是字节数。
         */
        m_CubeIBO.reset(
            Limen::IndexBuffer::Create(
                cubeIndices,
                sizeof(cubeIndices) / sizeof(uint32_t)
            )
        );

        m_CubeVAO->SetIndexBuffer(m_CubeIBO);

        /**
         * @brief 从外部GLSL文件创建3D Blinn-Phong Shader。
         *
         * 第一个参数：负责顶点变换和数据传递的Vertex Shader；
         * 第二个参数：负责纹理采样和光照计算的Fragment Shader。
         * 路径省略assets/shaders前缀；逻辑名称自动提取为BlinnPhong。
         */
        m_CubeShader = m_ShaderLib->Load(
            "OpenGL/Example3D/BlinnPhong.vert",
            "OpenGL/Example3D/BlinnPhong.frag"
        );

        LM_CORE_ASSERT(
            m_CubeShader,
            "Failed to create Example3D BlinnPhong shader"
        );

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

        //校验texture
        LM_CORE_ASSERT(m_AlbedoTexture,"Failed to create cube albedo texture");
    }

    void Example3DLayer::OnUpdate(Limen::DeltaTime &deltaTime)
    {
        // 本测试层暂时负责清理当前帧。
        Limen::RendererCommand::SetClearColor(
            {0.1f, 0.1f, 0.1f, 1.0f}
        );

        Limen::RendererCommand::Clear();

        /**
         * 开启深度测试。
         *
         * 立方体前面的片元应该遮挡后面的片元。
         */
        Limen::RendererCommand::SetDepthTest(true);

        /**
         * 使用透视相机开始3D场景。
         *
         * BeginScene会复制相机的ViewProjection矩阵和位置。
         */
        Limen::Renderer::BeginScene(m_Camera);

        // 使用DeltaTime实现与帧率无关的旋转。
        m_CubeRotationDegrees +=
                deltaTime.GetSeconds() *
                m_CubeRotationSpeed;

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

        m_AlbedoTexture->Bind(0);
        Limen::Renderer::Submit(
            m_CubeShader,
            *m_CubeVAO,
            cubeTransform
        );

        Limen::Renderer::EndScene();

        /**
         * 离开3D测试层前关闭深度测试，
         * 避免状态影响后续2D层和ImGui。
         */
        Limen::RendererCommand::SetDepthTest(false);
    }
} // SandBox
