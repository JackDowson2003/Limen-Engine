//
// Created by chenlong on 2026/8/26.
//
// 只使用 T*、T&、Ref<T> 声明
//    → 很多时候可以使用前向声明
//
// 需要创建对象、调用成员、使用 Scope<T> 析构
//    → 通常需要包含完整头文件
//
#include "Limen/Renderer/Renderer2D.h"

#include <glm/gtc/matrix_transform.hpp>
#include "Limen/RHI/Buffer.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/UniformBuffer.h"
#include "Limen/RHI/VertexArray.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/RHI/Texture.h"

namespace Limen
{
    /**
     * @brief Renderer2D 的私有的资源与运行状态
     *
     * 该结构只在Renderer2D.cpp中实现完整的定义，使公共头文件不需要
     * 暴露Shader、Pipeline、VertexArray
     */
    struct Renderer2D::Renderer2DData
    {
        /**
         * @brief 缓存 Renderer2D 使用的 Shader。
         *
         * ShaderLibrary 声明在 Shader 引用之前，因此会在其他成员
         * 销毁后最后销毁，保证 Shader 缓存的生命周期足够长。
         */
        ShaderLibrary Shaders;

        /**
        * @brief 所有二维 Quad 共享的顶点数组。
        *
        * VertexArray 记录公共 Quad 的顶点读取规则以及关联的
        * VertexBuffer 和 IndexBuffer。
        */
        Scope<VertexArray> QuadVertexArray;

        /**
         * @brief 保存公共 Quad 的顶点数据。
         */
        Ref<VertexBuffer> QuadVertexBuffer;

        /**
         * @brief 保存公共 Quad 的六个顶点索引。
         *
         * 两个三角形组成一个 Quad，索引通常为：
         * 0, 1, 2, 2, 3, 0。
         */
        Ref<IndexBuffer> QuadIndexBuffer;

        /**
         * @brief 绘制纯色 Quad 使用的 Shader。
         */
        Ref<Shader> FlatColorShader;

        /**
        * @brief 绘制纹理 Quad 使用的 Shader。
        */
        Ref<Shader> TextureShader;

        /**
        * @brief 纯色 Quad 使用的图形管线。
        *
        * 它会保存 FlatColorShader 以及二维绘制需要的深度、
        * 混合、剔除和图元拓扑状态。
        */
        Ref<GraphicsPipeline> FlatColorPipeline;

        /**
         * @brief 纹理 Quad 使用的图形管线。
         */
        Ref<GraphicsPipeline> TexturePipeline;

        /**
         * @brief 保存纯色 Quad 的 RGBA 颜色。
         *
         * glm::vec4 占用16字节，与 std140 中 vec4 的存储大小一致。
         */
        Scope<UniformBuffer> ColorUniformBuffer;

        /**
         * @brief Renderer2D 材质数据使用的 UniformBuffer 绑定位置。
         *
         * 约定：
         * 0 留给场景数据；
         * 1 留给物体数据；
         * 2 用于材质数据。
         *
         * 当前虽然还没有使用 SceneData 和 ObjectData UBO，
         * 但提前保留位置可以避免后续修改绑定约定。
         */
        static constexpr uint32_t MaterialUniformBinding = 2;

        /**
         * @brief Renderer2D 基础纹理使用的纹理槽。
         *
         * Texture Shader 的 u_Texture 与绘制时绑定的 Texture2D
         * 都必须使用相同的槽位。
         */
        static constexpr uint32_t TextureSlot = 0;

        /**
         * @brief 当前是否处于 BeginScene() 和 EndScene() 之间。
         *
         * 用于发现重复 BeginScene、遗漏 EndScene，以及在场景外
         * 调用 DrawQuad 等生命周期错误。
         */
        bool SceneActive = false;
    };

    /**
     * @brief 定义 Renderer2D 在头文件中声明的静态数据。
     *
     * 此时只创建一个空 Scope，不会创建任何 GPU 资源。
     * 真正的 Renderer2DData 将在 Init() 中创建。
     */
    Scope<Renderer2D::Renderer2DData> Renderer2D::s_Data = nullptr;


    void Renderer2D::Init()
    {
        LM_CORE_ASSERT(!s_Data, "Renderer2D already initialized!");

        if (s_Data)
            return;
        s_Data = CreateScope<Renderer2DData>();

        /*
         * 创建一个以局部原点为中心、宽高均为1的标准 Quad。
         *
         * Position 范围为 [-0.5, 0.5]，因此后续传入 size 时，
         * Model 矩阵缩放多少，Quad 的最终宽高就是多少。
         *
         * 每个顶点包含：
         * Position.xyz + TexCoord.uv，共5个float。
         */
        constexpr float quadVertices[] = {
            // Position              // TexCoord
            -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
            0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
            0.5f, 0.5f, 0.0f, 1.0f, 1.0f,
            -0.5f, 0.5f, 0.0f, 0.0f, 1.0f
        };

        /**
         * Create vao
         */
        s_Data->QuadVertexArray.reset(VertexArray::Create());

        LM_CORE_ASSERT(s_Data->QuadVertexArray, "Renderer2D: Vertex Array creation failed!");

        if (!s_Data->QuadVertexArray)
        {
            s_Data.reset();
            return;
        }

        s_Data->QuadVertexBuffer.reset(VertexBuffer::Create(quadVertices, sizeof(quadVertices)));
        LM_CORE_ASSERT(s_Data->QuadVertexBuffer, "Renderer2D: Vertex Buffer creation failed!");

        if (!s_Data->QuadVertexBuffer)
        {
            s_Data.reset();
            return;
        }
        /*
         * 告诉 VAO 每条顶点记录如何解析：
         *
         * stride = 3 * sizeof(float) + 2 * sizeof(float)
         *        = 20 bytes
         *
         * 不需要法线是因为 我们暂时不对这种东西做光照
         *
         * Position offset = 0
         * TexCoord offset = 12 bytes
         */
        const BufferLayout quadLayout{
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float2, "a_TexCoord"}
        };

        s_Data->QuadVertexBuffer->SetLayout(quadLayout);

        s_Data->QuadVertexArray->AddVertexBuffer(s_Data->QuadVertexBuffer);

        /*
         * 两个逆时针三角形组成一个 Quad：
         *
         * 3 -------- 2
         * |        / |
         * |      /   |
         * |    /     |
         * |  /       |
         * 0 -------- 1
         */
        constexpr uint32_t quadIndices[] = {
            0, 1, 2,
            2, 3, 0
        };

        s_Data->QuadIndexBuffer.reset(IndexBuffer::Create(quadIndices, sizeof(quadIndices) / sizeof(uint32_t)));

        LM_CORE_ASSERT(s_Data->QuadIndexBuffer, "Renderer2D,Failed to create Renderer2D quad IndexBuffer");
        if (!s_Data->QuadIndexBuffer)
        {
            s_Data.reset();
            return;
        }
        s_Data->QuadVertexArray->SetIndexBuffer(s_Data->QuadIndexBuffer);

        // 恢复干净的 VAO 绑定状态。
        s_Data->QuadVertexArray->UnBind();

        /**
         * 使用后端无关的逻辑路径去加载Renderer2D的内置Shader
         * OpenGL会自动解析为
         * assets/shaders/OpenGL/Renderer2D/FlatColor.vert
         * assets/shaders/OpenGL/Renderer2D/FlatColor.frag
         */
        s_Data->FlatColorShader = s_Data->Shaders.Load("Renderer2D/FlatColor");

        LM_CORE_ASSERT(s_Data->FlatColorShader, "Renderer2D: Shader load failed!");

        /**
         * 加载绘制纹理 Quad 所使用的Shader
         */
        s_Data->TextureShader = s_Data->Shaders.Load("Renderer2D/Texture2D");

        LM_CORE_ASSERT(s_Data->TextureShader, "Renderer2D: Shader load failed!");

        //release会不进行断言，因此需要进行保护
        if (!s_Data->FlatColorShader || !s_Data->TextureShader)
        {
            s_Data.reset();
            return;
        }

        //MaterialData 以后去绑定点2(MaterialUniformBinding)读取数据。
        s_Data->FlatColorShader->SetUniformBufferBinding(
            "MaterialData", //因为shader中是这个名字
            Renderer2DData::MaterialUniformBinding
        );

        /**
         * MaterialData中 当前值保存一个 RGBA 颜色
         * GLSL 的 std140布局中，vec4 占16 bytes
         * glm::vec4 同样保存4 个float
         */
        s_Data->ColorUniformBuffer = UniformBuffer::Create(
            sizeof(glm::vec4),
            Renderer2DData::MaterialUniformBinding
        );

        LM_CORE_ASSERT(
            s_Data->ColorUniformBuffer,
            "Renderer2D failed to create color UniformBuffer"
        );
        if (!s_Data->ColorUniformBuffer)
        {
            s_Data.reset();
            return;
        }

        s_Data->TextureShader->Bind();

        //从纹理槽0采样
        s_Data->TextureShader->SetInt("u_Texture", Renderer2DData::TextureSlot);

        s_Data->TextureShader->UnBind();

        /**
         * 创建纯色 Quad 使用的pipeline
         */
        GraphicsPipelineSpecification flatColorPipelineSpec;

        flatColorPipelineSpec.ShaderProgram = s_Data->FlatColorShader;

        flatColorPipelineSpec.Topology = PrimitiveTopology::TriangleList;

        // 当前基础 Renderer2D 按提交顺序绘制，不使用深度缓冲决定遮挡。
        flatColorPipelineSpec.DepthTestEnabled = false;
        flatColorPipelineSpec.DepthWriteEnabled = false;

        // RGBA 颜色需要支持透明度混合
        flatColorPipelineSpec.Blend = BlendMode::AlphaBlend;

        // 2D Quad 不进行正反面提出 避免负缩放或者反转后消失
        flatColorPipelineSpec.Culling = CullMode::None;

        // 当前不剔除背面，也去记录默认围绕正面的绕序
        flatColorPipelineSpec.FrontFaceWinding = FrontFace::CounterClockwise;

        // 用于日志、调试和未来的 Pipeline 缓存。
        flatColorPipelineSpec.DebugName =
                "Renderer2D Flat Color Pipeline";

        s_Data->FlatColorPipeline = GraphicsPipeline::Create(flatColorPipelineSpec);

        LM_CORE_ASSERT(
            s_Data->FlatColorPipeline,
            "Renderer2D failed to create flat color pipeline"
        );

        if (!s_Data->FlatColorPipeline)
        {
            s_Data.reset();
            return;
        }

        /*
 * 创建纹理 Quad 使用的图形管线规格。
 *
 * 固定功能状态与纯色 Quad 相同，主要区别是使用的 Shader 不同。
 */
        GraphicsPipelineSpecification texturePipelineSpecification{};

        texturePipelineSpecification.ShaderProgram =
                s_Data->TextureShader;

        texturePipelineSpecification.Topology =
                PrimitiveTopology::TriangleList;

        texturePipelineSpecification.DepthTestEnabled = false;
        texturePipelineSpecification.DepthWriteEnabled = false;

        texturePipelineSpecification.Blend =
                BlendMode::AlphaBlend;

        texturePipelineSpecification.Culling =
                CullMode::None;

        texturePipelineSpecification.FrontFaceWinding =
                FrontFace::CounterClockwise;

        texturePipelineSpecification.DebugName =
                "Renderer2D Texture Pipeline";

        s_Data->TexturePipeline =
                GraphicsPipeline::Create(texturePipelineSpecification);

        LM_CORE_ASSERT(
            s_Data->TexturePipeline,
            "Renderer2D failed to create texture pipeline"
        );

        if (!s_Data->TexturePipeline)
        {
            s_Data.reset();
            return;
        }
    }

    void Renderer2D::Shutdown()
    {
        if (!s_Data)
            return;

        LM_CORE_ASSERT(!s_Data->SceneActive,
                       "Renderer2D cannot shut down while a scene is active");

        /*
         * Release 构建中断言可能关闭，所以仍然安全结束场景。
         */
        if (s_Data->SceneActive)
        {
            Renderer::EndScene();
            s_Data->SceneActive = false;
        }

        s_Data.reset();
    }

    void Renderer2D::BeginScene(const Camera &camera)
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::Init() must be called before BeginScene()"
        );
        if (!s_Data)
            return;

        LM_CORE_ASSERT(!s_Data->SceneActive,
                       "Renderer2D::BeginScene() called when Scene is active");

        if (s_Data->SceneActive)
            return;

        /*
         * 把相机交给通用 Renderer。
         * Renderer 会保存 ViewProjection 和 CameraPosition。
         */
        Renderer::BeginScene(camera);

        s_Data->SceneActive = true;
    }

    void Renderer2D::EndScene()
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::Init() must be called before EndScene()"
        );

        if (!s_Data)
            return;

        LM_CORE_ASSERT(
            s_Data->SceneActive,
            "Renderer2D::EndScene() called without BeginScene()"
        );

        if (!s_Data->SceneActive)
            return;

        Renderer::EndScene();

        s_Data->SceneActive = false;
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        /*
    * 标准 Quad 以局部原点为中心，初始宽高均为1。
    *
    * Scale 将它缩放为指定宽高；
    * Translate 将缩放后的 Quad 移动到世界空间位置。
    *
    * GLM 使用列向量，因此矩阵组合顺序写为 T × S。
    */
        const glm::mat4 transform =
                glm::translate(
                    glm::mat4(1.0f),
                    position
                ) *
                glm::scale(
                    glm::mat4(1.0f),
                    glm::vec3(size, 1.0f)
                );

        DrawQuad(
            transform,
            color
        );
    }

    void Renderer2D::DrawQuad(const glm::mat4 &transform, const glm::vec4 &color)
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::Init() must be called before DrawQuad()"
        );

        if (!s_Data)
            return;

        LM_CORE_ASSERT(
            s_Data->SceneActive,
            "Renderer2D::DrawQuad() must be called between BeginScene() and EndScene()"
        );

        if (!s_Data->SceneActive)
            return;
        /*
         * 更新绑定点2中的颜色 UBO。
         * Fragment Shader 的 MaterialData 会从这里读取 u_Color。
         */
        s_Data->ColorUniformBuffer->SetData(&color, sizeof(color));

        //使用纯色pipeline 和 公共的vao来提交
        Renderer::Submit(
            *s_Data->FlatColorPipeline,
            *s_Data->QuadVertexArray,
            transform
        );
    }

    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const Ref<Texture2D> &texture)
    {
        /*
        * 与纯色 Quad 使用相同的 Model 矩阵构造方式：
        * 先在局部空间缩放，再移动到世界空间。
        */
        const glm::mat4 transform =
                glm::translate(
                    glm::mat4(1.0f),
                    position
                ) *
                glm::scale(
                    glm::mat4(1.0f),
                    glm::vec3(size, 1.0f)
                );

        DrawQuad(
            transform,
            texture
        );
    }

    /**
     *
     * DrawQuad(position, size, texture)
     *        ↓
     * 生成 Model = Translate × Scale
     *        ↓
     * DrawQuad(transform, texture)
     *        ↓
     * texture->Bind(0)
     *        ↓
     * TexturePipeline.Bind()
     *        ↓
     * TextureShader 中 u_Texture 从槽0采样
     *        ↓
     * Renderer::Submit()
     *        ↓
     * 绘制公共 Quad VAO
     * @param transform transform
     * @param texture texture of the object
     */
    void Renderer2D::DrawQuad(const glm::mat4 &transform, const Ref<Texture2D> &texture)
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::Init() must be called before DrawQuad()"
        );

        if (!s_Data)
            return;

        LM_CORE_ASSERT(s_Data->SceneActive,
                       "Renderer2D::DrawQuad() must be called between BeginScene() and EndScene()"
        );

        if (!s_Data->SceneActive)
            return;

        LM_CORE_ASSERT(
            texture,
            "Renderer2D::DrawQuad() received a null texture"
        );

        if (!texture)
            return;

        /**
         * 把真实纹理绑定到纹理槽
         * TextureShader初始化时已经设定
         * u_Texture = 0
         */
        texture->Bind(Renderer2DData::TextureSlot);

        Renderer::Submit(
            *s_Data->TexturePipeline,
            *s_Data->QuadVertexArray,
            transform
        );
    }
}
