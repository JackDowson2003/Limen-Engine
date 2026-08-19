//
// Created by chenlong on 2026/8/20.
//

#include "Example3DLayer.h"

#include "Renderer/Renderer.h"
#include "glm/glm.hpp"
#include <glm/ext/matrix_transform.hpp>


namespace SandBox
{
    Example3DLayer::Example3DLayer()
        : Layer("3D Layer"),
          m_Camera(
              45.f, 1600.f / 900.f, 0.1f, 100.f
          )
    {
        /**
         * 立方体的8个角点。
         *
         * 立方体中心位于局部空间原点，
         * 边长为1，每个坐标分量范围是[-0.5, 0.5]。
         */
        constexpr float cubeVertices[] = {
            -0.5f, -0.5f, -0.5f, // 0
             0.5f, -0.5f, -0.5f, // 1
             0.5f,  0.5f, -0.5f, // 2
            -0.5f,  0.5f, -0.5f, // 3

            -0.5f, -0.5f,  0.5f, // 4
             0.5f, -0.5f,  0.5f, // 5
             0.5f,  0.5f,  0.5f, // 6
            -0.5f,  0.5f,  0.5f  // 7
        };

        /**
        * 立方体索引。
        *
        * 6个面 × 每面2个三角形 × 每个三角形3个索引
        * = 36个索引。
        */
        constexpr uint32_t cubeIndices[] = {
            // 前面：z = +0.5
            4, 5, 6,
            6, 7, 4,

            // 后面：z = -0.5
            1, 0, 3,
            3, 2, 1,

            // 左面：x = -0.5
            0, 4, 7,
            7, 3, 0,

            // 右面：x = +0.5
            5, 1, 2,
            2, 6, 5,

            // 上面：y = +0.5
            3, 7, 6,
            6, 2, 3,

            // 下面：y = -0.5
            0, 1, 5,
            5, 4, 0
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
         * 描述每个顶点的数据结构。
         *
         * Float3表示每个顶点包含3个float：
         *
         *     x, y, z
         *
         * 对应顶点Shader中的location = 0。
         */
        const Limen::BufferLayout cubeLayout
        {
            {
                Limen::ShaderDataType::Float3,
                "a_Position"
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

        const std::string vertexSource = R"(
            #version 410 core

            /**
             * 立方体顶点的局部空间位置。
             * 对应cubeLayout中的Float3。
             */
            layout(location = 0) in vec3 a_Position;

            // Projection × View，由当前Camera提供。
            uniform mat4 u_ViewProjection;

            // 当前立方体的Model矩阵。
            uniform mat4 u_Transform;

            // 传递给片元Shader，用于生成调试颜色。
            out vec3 v_LocalPosition;

            void main()
            {
                v_LocalPosition = a_Position;

                gl_Position =
                    u_ViewProjection *
                    u_Transform *
                    vec4(a_Position, 1.0);
            }
        )";

        const std::string fragmentSource = R"(
            #version 410 core

            layout(location = 0) out vec4 color;

            in vec3 v_LocalPosition;

            void main()
            {
                /**
                 * 把局部坐标映射为RGB颜色，
                 * 方便观察立方体的方向和插值结果。
                 */
                vec3 debugColor =
                    v_LocalPosition * 0.5 + 0.5;

                color = vec4(debugColor, 1.0);
            }
        )";

        m_CubeShader = Limen::Shader::Create(
            vertexSource,
            fragmentSource
        );
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
        const glm::mat4 cubeTransform = glm::rotate(
            glm::mat4(1.0f),
            glm::radians(m_CubeRotationDegrees),
            glm::vec3(0.0f, 1.0f, 0.0f)
        );

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
