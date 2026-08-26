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
#include <array>
#include "Limen/RHI/VertexBuffer.h"
#include "Limen/RHI/IndexBuffer.h"
#include "Limen/RHI/GraphicsPipeline.h"
#include "Limen/RHI/Shader.h"
#include "Limen/RHI/VertexArray.h"
#include "Limen/Core/Log.h"
#include "Limen/Renderer/Renderer.h"
#include "Limen/RHI/Texture.h"

namespace Limen
{
    namespace
    {
        /**
         * @brief Renderer2D批处理使用的单个顶点
         * 批处理不会再为了单个Quad去上传Transform和Color
         * CPU会提前计算顶点的世界坐标，然后将这一批点点一次上传到GPU
         */
        struct QuadVertex
        {
            glm::vec3 Position{0.0f};
            glm::vec4 Color{1.0f};
            glm::vec2 TexCoord{0.0f};
            uint32_t TextureIndex = 0;
        };
    }

    /**
     * @brief 一个批次最多容纳的 Quad 数量。
     *
     * 达到上限后，Renderer2D 会先提交当前批次，
     * 然后开始收集下一个批次。
     */
    static constexpr uint32_t MaxQuads = 10000;

    static constexpr uint32_t VerticesPerQuad = 4;
    static constexpr uint32_t IndicesPerQuad = 6;

    static constexpr uint32_t MaxVertices =
            MaxQuads * VerticesPerQuad;

    static constexpr uint32_t MaxIndices =
            MaxQuads * IndicesPerQuad;

    /**
     * @brief 一个批次最多同时使用的二维纹理数量。
     *
     * OpenGL 4.1 至少支持16个 Fragment Shader 纹理单元。
     * 后续可以通过 RendererCapabilities 查询硬件上限，
     * 当前先使用跨设备更稳定的16。
     */
    static constexpr uint32_t MaxTextureSlots = 16;

    /**
     * @brief 表示当前 Quad 不采样纹理，只输出顶点颜色。
     *
     * 有效纹理槽范围是 [0, MaxTextureSlots - 1]，
     * 所以 MaxTextureSlots 本身可以作为特殊保留值。
     */
    static constexpr uint32_t NoTextureIndex =
            MaxTextureSlots;

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
         * @brief 单位 Quad 的四个局部空间顶点。
         *
         * DrawQuad 时会使用每个 Quad 的 Model 矩阵，
         * 将它们转换为世界空间坐标并写入批处理顶点缓存。
         * 这里使用 vec4 是因为稍后要直接进行：transform * QuadVertexPositions[i]
         */
        std::array<glm::vec4, VerticesPerQuad> QuadVertexPositions{};

        /**
         * @brief 单位 Quad 四个顶点对应的纹理坐标。
         *
         * 顺序必须和 QuadVertexPositions 完全一致：
         * 左下、右下、右上、左上。
         */
        std::array<glm::vec2, VerticesPerQuad>
        QuadTextureCoordinates{};

        /**
         * @brief CPU 端批处理顶点数组的所有者。
         *
         * 它拥有一块最多容纳 MaxVertices 个 QuadVertex 的连续内存。
         * 每帧先把所有 Quad 顶点写到这里，再一次上传到 GPU VBO。
         */
        Scope<QuadVertex[]> QuadVertexBufferBase;

        /**
         * @brief CPU 顶点数组当前的写入位置。
         *
         * 这是非拥有指针，只负责指向下一处可以写入 QuadVertex 的位置。
         * 每次开始新批次时，它会重新指向 QuadVertexBufferBase 的开头。
         */
        QuadVertex *QuadVertexBufferWritePointer = nullptr;

        /**
         * @brief 当前批次已经写入的索引数量。
         *
         * 每加入一个 Quad 增加6。
         * 达到 MaxIndices 时必须提交当前批次并开始新批次。
         */
        uint32_t QuadIndexCount = 0;

        /**
         * @brief 保存公共 Quad 的六个顶点索引。
         *
         * 两个三角形组成一个 Quad，索引通常为：
         * 0, 1, 2, 2, 3, 0。
         */
        Ref<IndexBuffer> QuadIndexBuffer;

        /**
        * @brief 绘制纹理 Quad 使用的 Shader。
        */
        Ref<Shader> TextureShader;


        /**
         * @brief 纹理 Quad 使用的图形管线。
         */
        Ref<GraphicsPipeline> TexturePipeline;

        /**
         * @brief 保存纯色 Quad 的 RGBA 颜色。
         *
         * glm::vec4 占用16字节，与 std140 中 vec4 的存储大小一致。
         */
        // Scope<UniformBuffer> ColorUniformBuffer; //弃用

        /**
         * @brief 当前二维场景使用的 ViewProjection 矩阵。
         *
         * 批处理中的顶点已经在 CPU 端变换到了世界空间，
         * Shader 只需要再乘一次 ViewProjection。
         */
        glm::mat4 ViewProjection{1.0f};

        /**
         * @brief 当前批次使用的所有二维纹理。
         *
         * 数组下标就是写入 QuadVertex::TextureIndex 的值。
         * Ref 保证纹理在本批次提交完成前不会被销毁。
         */
        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots{};

        /**
         * @brief 当前批次已经占用的纹理槽数量。
         *
         * 有效纹理槽范围是 [0, TextureSlotCount)。
         */
        uint32_t TextureSlotCount = 0;

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

    //StartBatch() 不会重新申请内存，只是把状态归零，所以开销很小。
    void Renderer2D::StartBatch()
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::StartBatch requires initialized Renderer2D"
        );

        if (!s_Data)
            return;

        //新批次还没有索引
        s_Data->QuadIndexCount = 0;

        //从CPU 顶点数组 的第一个位置重新开始写入
        s_Data->QuadVertexBufferWritePointer = s_Data->QuadVertexBufferBase.get();

        s_Data->TextureSlots.fill(nullptr);
        s_Data->TextureSlotCount = 0;
    }

    void Renderer2D::Flush()
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::Flush requires initialized Renderer2D"
        );

        if (!s_Data)
            return;

        if (s_Data->QuadIndexCount == 0)
            return;

        /*
         * 当前写指针减去数组起始地址，
         * 得到本批次实际写入了多少个 QuadVertex。
         *
         * 指针相减的结果单位是 QuadVertex，而不是字节。
         */
        const uint32_t vertexCount = static_cast<uint32_t>(
            s_Data->QuadVertexBufferWritePointer
            - s_Data->QuadVertexBufferBase.get()
        );

        // SetData() 接收的是字节数，因此需要乘单个顶点大小。
        const uint32_t dataSize =
                static_cast<uint32_t>(
                    vertexCount * sizeof(QuadVertex)
                );

        // 将本批次实际使用的顶点上传到动态 GPU VBO。
        s_Data->QuadVertexBuffer->SetData(
            s_Data->QuadVertexBufferBase.get(),
            dataSize,
            0
        );

        /*
         * 将当前批次使用的所有纹理绑定到对应纹理槽。
         *
         * TextureSlots 的数组下标必须与
         * QuadVertex::TextureIndex 完全一致。
         */
        for (uint32_t textureSlot = 0;
             textureSlot < s_Data->TextureSlotCount;
             ++textureSlot)
        {
            LM_CORE_ASSERT(
                s_Data->TextureSlots[textureSlot],
                "Renderer2D batch contains a null texture"
            );

            if (!s_Data->TextureSlots[textureSlot])
                continue;

            s_Data->TextureSlots[textureSlot]->Bind(
                textureSlot
            );
        }

        /*
         * 纯色 Quad 和纹理 Quad 统一使用 TexturePipeline。
         * Shader 会根据 TextureIndex 判断是否采样纹理。
         */
        s_Data->TexturePipeline->Bind();

        s_Data->TextureShader->SetMat4(
            "u_ViewProjection",
            s_Data->ViewProjection
        );

        s_Data->QuadVertexArray->Bind();

        RendererCommand::DrawIndexed(
            *s_Data->QuadVertexArray,
            PrimitiveTopology::TriangleList,
            s_Data->QuadIndexCount
        );
    }

    void Renderer2D::NextBatch()
    {
        //先上传已经手机的quad
        Flush();

        //清空计数器和指针
        StartBatch();
    }


    void Renderer2D::Init()
    {
        LM_CORE_INFO("Running Renderer2D::Init()");

        LM_CORE_ASSERT(!s_Data, "Renderer2D already initialized!");

        if (s_Data)
            return;

        s_Data = CreateScope<Renderer2DData>();

        /**
         * Create vao
         */
        s_Data->QuadVertexArray.reset(VertexArray::Create());

        s_Data->QuadVertexPositions = {
            glm::vec4(-0.5f, -0.5f, 0.0f, 1.0f),
            glm::vec4(0.5f, -0.5f, 0.0f, 1.0f),
            glm::vec4(0.5f, 0.5f, 0.0f, 1.0f),
            glm::vec4(-0.5f, 0.5f, 0.0f, 1.0f)
        };

        s_Data->QuadTextureCoordinates = {
            glm::vec2(0.0f, 0.0f),
            glm::vec2(1.0f, 0.0f),
            glm::vec2(1.0f, 1.0f),
            glm::vec2(0.0f, 1.0f)
        };

        s_Data->QuadVertexBufferBase =
                CreateScope<QuadVertex[]>(MaxVertices);

        s_Data->QuadVertexBufferWritePointer =
                s_Data->QuadVertexBufferBase.get();

        LM_CORE_ASSERT(s_Data->QuadVertexArray, "Renderer2D: Vertex Array creation failed!");

        if (!s_Data->QuadVertexArray)
        {
            s_Data.reset();
            return;
        }

        s_Data->QuadVertexBuffer.reset(VertexBuffer::Create(
            MaxVertices * sizeof(QuadVertex)
        ));
        LM_CORE_ASSERT(s_Data->QuadVertexBuffer, "Renderer2D: Vertex Buffer creation failed!");

        if (!s_Data->QuadVertexBuffer)
        {
            s_Data.reset();
            return;
        }

        /*
         * QuadVertex 的顶点布局：
         *
         * Position     Float3：12字节，Offset = 0
         * Color        Float4：16字节，Offset = 12
         * TexCoord     Float2： 8字节，Offset = 28
         * TextureIndex UInt：   4字节，Offset = 36
         *
         * 总 Stride = 40字节。
         */
        const VertexBufferLayout quadLayout{
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float4, "a_Color"},
            {ShaderDataType::Float2, "a_TexCoord"},
            {ShaderDataType::UInt, "a_TextureIndex"}
        };


        s_Data->QuadVertexBuffer->SetLayout(quadLayout);

        s_Data->QuadVertexArray->AddVertexBuffer(s_Data->QuadVertexBuffer);

        /*
         * 为整个 Renderer2D 批次创建索引数组。
         *
         * 每个 Quad 由4个顶点和2个三角形组成：
         *
         * 3 -------- 2
         * |        / |
         * |      /   |
         * |    /     |
         * |  /       |
         * 0 -------- 1
         *
         * 两个三角形的索引顺序为：
         * 0, 1, 2
         * 2, 3, 0
         *
         * 下一个 Quad 使用后面的4个顶点，因此索引需要整体增加4。
         */

        // 在 CPU 端创建能够容纳整个批次索引的临时数组。
        // MaxIndices = MaxQuads * 6。
        Scope<uint32_t[]> quadIndices = CreateScope<uint32_t[]>(MaxIndices);

        uint32_t vertexOffset = 0;
        for (uint32_t index = 0; index < MaxIndices; index += IndicesPerQuad)
        {
            // 第一个三角形：
            // 左下角 → 右下角 → 右上角。
            quadIndices[index + 0] = vertexOffset + 0;
            quadIndices[index + 1] = vertexOffset + 1;
            quadIndices[index + 2] = vertexOffset + 2;

            // 第二个三角形：
            // 右上角 → 左上角 → 左下角。
            quadIndices[index + 3] = vertexOffset + 2;
            quadIndices[index + 4] = vertexOffset + 3;
            quadIndices[index + 5] = vertexOffset + 0;

            // 一个 Quad 使用4个独立顶点。
            // 下一次循环从下一组4个顶点开始生成索引。
            vertexOffset += VerticesPerQuad;
        }

        // 创建GPU端的IndexBuffer
        // 这里传入的是索引数量 MaxIndices，不是字节数
        s_Data->QuadIndexBuffer.reset(IndexBuffer::Create(quadIndices.get(), MaxIndices));


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
         * 加载绘制纹理 Quad 所使用的Shader
         */
        s_Data->TextureShader = s_Data->Shaders.Load("Renderer2D/Texture2D");

        LM_CORE_ASSERT(s_Data->TextureShader, "Renderer2D batch Shader load failed");

        //release会不进行断言，因此需要进行保护
        if ( !s_Data->TextureShader)
        {
            s_Data.reset();
            return;
        }
        s_Data->TextureShader->Bind();

        /*
         * 告诉 Shader：
         *
         * u_Textures[0] 采样纹理单元0，
         * u_Textures[1] 采样纹理单元1，
         * ...
         * u_Textures[15] 采样纹理单元15。
         *
         * 这项映射只需要在 Shader 初始化时设置一次。
         */
        for (uint32_t textureSlot = 0; textureSlot < MaxTextureSlots; ++textureSlot)
        {
            const std::string uniformName = "u_Textures[" + std::to_string(textureSlot) + "]";

            s_Data->TextureShader->SetInt(uniformName.c_str(), textureSlot);
        }

        s_Data->TextureShader->UnBind();

        /**
         * 创建纯色 Quad 使用的pipeline
         */
        GraphicsPipelineSpecification flatColorPipelineSpec;


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
        LM_CORE_INFO("Running Renderer2D::BeginScene()");

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

        // 保存本次二维场景使用的相机矩阵。
        s_Data->ViewProjection =
                camera.GetViewProjectionMatrix();

        // 清空上一批数据，开始收集当前场景的 Quad。
        StartBatch();

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


        // DrawQuad() 只向 CPU 缓存写入顶点。
        // 到 EndScene() 时统一上传并绘制剩余批次。
        Flush();

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
         * 当前批次如果已经没有空间容纳一个完整 Quad，
         * 就先提交当前批次，再开始收集下一批。
         *
         * 一个 Quad 必须完整地放在同一个批次中，
         * 不能只写入一部分顶点或索引。
         */
        if (s_Data->QuadIndexCount + IndicesPerQuad > MaxIndices)
        {
            NextBatch();
        }

        /*
         * 一个 Quad 有4个顶点。
         *
         * QuadVertexPositions 保存的是局部空间单位 Quad，
         * transform 将每个顶点转换到世界空间。
         */
        for (uint32_t vertexIndex = 0; vertexIndex < VerticesPerQuad; vertexIndex++)
        {
            // 取得 CPU 顶点数组中当前可以写入的位置。
            auto &[Position, Color, TexCoord, TextureIndex] =
                *s_Data->QuadVertexBufferWritePointer;

            //计算世界空间的位置
            const glm::vec4 &worldPosition = transform * s_Data->QuadVertexPositions[vertexIndex];

            Position = glm::vec3(worldPosition);

            Color = color;
            /*
             * 纯色 Shader 暂时不会读取纹理字段。
             * 仍然初始化它们，避免 CPU 缓存中保留无意义旧数据。
             */
            TexCoord = glm::vec2(0.0f);
            // vertex.TextureIndex = 0;
            /*
             * 该 Quad 是纯色 Quad。
             * Fragment Shader 看到 NoTextureIndex 后不会采样纹理。
             */
            TextureIndex = NoTextureIndex;

            s_Data->QuadVertexBufferWritePointer++;
        }
        s_Data->QuadIndexCount += IndicesPerQuad;
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

        /*
         * 如果当前批次已经无法容纳一个完整 Quad，
         * 先提交当前批次并开始下一批。
         */
        if (s_Data->QuadIndexCount + IndicesPerQuad > MaxIndices)
        {
            NextBatch();
        }
        /*
         * 先在当前批次已经使用的纹理中查找。
         *
         * 同一个 Ref<Texture2D> 被多个 Quad 使用时，
         * 所有 Quad 应当复用同一个纹理槽。
         */
        uint32_t textureIndex = NoTextureIndex;

        //看看是否有和当前texture使用的一致的
        for (uint32_t textureSlot = 0;
             textureSlot < s_Data->TextureSlotCount;
             ++textureSlot)
        {
            if (
                s_Data->TextureSlots[textureSlot] ==
                texture
            )
            {
                textureIndex = textureSlot;
                break;
            }
        }

        /*
         * 没找到说明该纹理还没有加入当前批次。
         */
        if (textureIndex == NoTextureIndex)
        {
            /*
             * 当前批次的纹理槽已经用完。
             *
             * 必须先提交当前批次，新的批次会重新获得
             * MaxTextureSlots 个可用纹理槽。
             */
            if (s_Data->TextureSlotCount >= MaxTextureSlots)
            {
                NextBatch();
            }
            // 使用下一个空闲纹理槽。 此时如果用完了就为0
            textureIndex = s_Data->TextureSlotCount;

            //保存ref
            s_Data->TextureSlots[textureIndex] = texture;

            s_Data->TextureSlotCount++;
        }

        //把纹理槽 Quad的4个顶点写入CPU
        for (uint32_t vertexIndex = 0; vertexIndex < VerticesPerQuad; ++vertexIndex)
        {
            auto &[Position, Color, TexCoord, TextureIndex]
                    = *s_Data->QuadVertexBufferWritePointer;

            // 单位Quad转换到世界坐标
            const glm::vec4 worldPosition = transform * s_Data->QuadVertexPositions[vertexIndex];

            Position = glm::vec3(worldPosition);
            // 白色不会改变纹理原色：
            Color = glm::vec4(1.0f);

            TexCoord = s_Data->QuadTextureCoordinates[vertexIndex];

            TextureIndex = textureIndex;

            ++s_Data->QuadVertexBufferWritePointer;
        }
        s_Data->QuadIndexCount += IndicesPerQuad;
    }
}
