//
// Created by chenlong on 2026/8/20.
//

#include "Example3DLayer.h"

#include "Renderer/Renderer.h"
#include "glm/glm.hpp"
#include <glm/ext/matrix_transform.hpp>

#include "Log.h"


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

        const std::string vertexShaderSource = R"(
            #version 410 core

            // 顶点在模型局部空间中的位置。
            layout(location = 0) in vec3 a_Position;

            // 顶点在模型局部空间中的法线。
            layout(location = 1) in vec3 a_Normal;

            /**
             * 顶点的二维纹理坐标。
             *
             * location = 2 对应BufferLayout中的第三个元素：
             *
             *     Float2, "a_TexCoord"
             *
             * x分量表示u，水平方向；
             * y分量表示v，垂直方向。
             */
            layout(location = 2) in vec2 a_TexCoord;

            // 当前相机的Projection × View矩阵。
            uniform mat4 u_ViewProjection;

            // 当前物体从局部空间到世界空间的Model矩阵。
            uniform mat4 u_Transform;

            // 传递给片元Shader的世界空间法线。
            out vec3 v_WorldNormal;

            // 传递给片元Shader的着色点世界坐标。
            out vec3 v_WorldPosition;

            /**
             * 传递给Fragment Shader的纹理坐标。
             *
             * GPU会在三角形三个顶点之间自动进行透视正确插值，
             * 因此每个片元都会得到属于自己的UV坐标。
             */
            out vec2 v_TexCoord;

            void main()
            {
                /**
                 * 法线不能直接乘Model矩阵。
                 *
                 * 当物体存在非均匀缩放时，需要使用：
                 *
                 *     transpose(inverse(mat3(Model)))
                 *
                 * 才能保证变换后的法线仍然垂直于表面。
                 */
                mat3 normalMatrix =
                    transpose(inverse(mat3(u_Transform)));

                //因为逆-转秩后法线长度会被缩放 所以必须让法线进行归一化，否则光照亮度直接乱掉
                v_WorldNormal =
                    normalize(normalMatrix * a_Normal);

                v_TexCoord = a_TexCoord;


                vec4 worldPosition =
                    u_Transform * vec4(a_Position, 1.0);

                v_WorldPosition = worldPosition.xyz;

                gl_Position =
                    u_ViewProjection * worldPosition;
            }
        )";
        const std::string fragmentShaderSource = R"(
            #version 410 core

            layout(location = 0) out vec4 color;

            in vec3 v_WorldNormal;
            in vec3 v_WorldPosition;

            /**
             * 由Vertex Shader输出并经过光栅化插值后的纹理坐标。
             */
            in vec2 v_TexCoord;

            // 相机在世界空间中的位置，由Renderer::Submit上传。
            uniform vec3 u_CameraPosition;

            /**
             * @brief 立方体材质的Albedo纹理采样器。
             *
             * sampler2D本身不保存图片数据，
             * 它记录Shader应该从哪个二维纹理槽读取数据。
             *
             * 当前约定它读取纹理槽0。
             */
            uniform sampler2D u_AlbedoTexture;

            void main()
            {


                vec4 albedoSample = texture(u_AlbedoTexture, v_TexCoord);


                 vec3 k_d = albedoSample.rgb;

                 vec3 k_a =
                    0.15 * k_d;

                vec3 k_s =
                    vec3(0.35);

                /**
                 * p：Blinn-Phong高光指数。
                 *
                 * p越大，高光越小、越集中；
                 * p越小，高光越宽、看起来越粗糙。
                 */
                const float p = 64.0;

                // GAMES101公式中的环境光强度 I_a。
                const vec3 ambientLightIntensity =
                    vec3(1.0);

                /**
                 * 当前使用平行光，没有光源位置和距离r，
                 * 因此不使用1 / r^2距离衰减。
                 */
                const vec3 lightIntensity =vec3(1.0, 0.95, 0.85);

                /**
                 * 光线在世界空间中的传播方向。
                 *
                 * 这里表示光线从左上前方向右下后方传播。
                 */
                const vec3 lightRayDirection =
                    normalize(vec3(1.0, -1.0, -1.0));

                // n：世界空间中单位长度的表面法线。
                vec3 n =
                    normalize(v_WorldNormal);

                vec3 l = -lightRayDirection;

                /**
                 * Lambert漫反射：
                 *
                 *     max(dot(N, L), 0)
                 *
                 * N：表面法线；
                 * L：表面指向光源的方向。
                 */
                float nDotL = max(
                    dot(n, l),
                    0.0
                );

                /**
                 * v：从当前着色点指向相机的单位方向。
                 *
                 * 两个位置相减先得到方向，然后归一化。
                 */
                vec3 v = normalize(
                    u_CameraPosition - v_WorldPosition
                );

                /**
                 * h：Blinn-Phong半程向量。
                 *
                 * h位于光照方向l与观察方向v之间。
                 * 当表面法线n越接近h，镜面高光越强。
                 */
                vec3 h = normalize(l + v);

                float nDotH = max(
                    dot(n, h),
                    0.0
                );

                // L_a = k_a * I_a
                vec3 ambient =
                    k_a * ambientLightIntensity;

                // L_d = k_d * I * max(0, n dot l)
                vec3 diffuse =
                    k_d * lightIntensity * nDotL;

                /**
                 * L_s = k_s * I * max(0, n dot h)^p
                 *
                 * 当光源在表面背面时，nDotL为0，
                 * 镜面反射也应该为0，避免背面出现错误高光。
                 */
                float specularStrength =
                    nDotL > 0.0
                        ? pow(nDotH, p)
                        : 0.0;

                vec3 specular =
                    k_s * lightIntensity * specularStrength;

                color = vec4(
                    ambient + diffuse + specular,
                    albedoSample.a
                );
            }
        )";


        m_CubeShader = Limen::Shader::Create(
            vertexShaderSource,
            fragmentShaderSource
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
