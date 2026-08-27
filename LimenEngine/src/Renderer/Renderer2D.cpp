//
// Created by chenlong on 2026/8/26.
//
// Renderer2D.cpp 负责二维批处理渲染器的具体实现。
//
// 头文件只通过 T*、T&、Ref<T> 声明 Camera 和 Texture2D，
// 因此可以使用前向声明，减少公共头文件依赖。
// 本文件需要创建对象、调用成员函数并析构 Scope<T>/Ref<T>，
// 所以必须包含相关类型的完整定义。
//
// 一帧二维场景的数据流：
//
// BeginScene
//     -> StartBatch：清空 CPU 批次状态
// DrawQuad（可调用很多次）
//     -> 计算世界空间顶点
//     -> 写入 CPU 顶点缓存
//     -> 累加索引数量
// EndScene
//     -> Flush：上传 VBO、绑定纹理和 Pipeline、执行 DrawIndexed
//
#include "Limen/Renderer/Renderer2D.h"

// glm::translate 和 glm::scale 的定义。
#include <glm/gtc/matrix_transform.hpp>

// Renderer2DData 使用固定大小数组保存单位 Quad 和纹理槽。
#include <array>

// 以下头文件只放在 cpp 中，避免把具体 RHI 资源暴露给 Renderer2D.h。
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
    // 匿名命名空间中的名字只在当前 cpp 内可见，
    // QuadVertex 是 Renderer2D 的内部 CPU/GPU 顶点格式，不属于公共接口。
    namespace
    {
        /**
         * @brief Renderer2D 批处理使用的单个顶点格式。
         *
         * 批处理不会为每个 Quad 单独上传 Model 矩阵和颜色。
         * DrawQuad() 会在 CPU 上把单位 Quad 变换到世界空间，
         * 然后把多个 Quad 的顶点连续写入同一块 CPU 内存。
         * Flush() 再把整段有效数据一次上传到 GPU。
         *
         * 字段与 Renderer2D Shader 的顶点输入一一对应：
         *
         * location 0 -> Position
         * location 1 -> Color
         * location 2 -> TexCoord
         * location 3 -> TextureIndex
         */
        struct QuadVertex
        {
            // 已经经过 Model 矩阵变换的世界空间位置。
            glm::vec3 Position{0.0f};

            // 顶点 RGBA 颜色；纹理 Quad 当前写入白色，避免改变纹理原色。
            glm::vec4 Color{1.0f};

            // 纹理坐标；纯色 Quad 不采样纹理，但仍会初始化该字段。
            glm::vec2 TexCoord{0.0f};

            // 当前批次中的纹理槽下标；NoTextureIndex 表示只输出颜色。
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

    /**
     * @brief 一个 Quad 固定由四个独立顶点组成。
     */
    static constexpr uint32_t VerticesPerQuad = 4;

    /**
     * @brief 一个 Quad 被拆成两个三角形，因此固定需要六个索引。
     */
    static constexpr uint32_t IndicesPerQuad = 6;

    /**
     * @brief CPU 顶点缓存和动态 VBO 能够容纳的最大顶点数。
     */
    static constexpr uint32_t MaxVertices =
            MaxQuads * VerticesPerQuad;

    /**
     * @brief 静态 IBO 中预先生成的最大索引数。
     */
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
    static constexpr uint32_t NoTextureIndex = MaxTextureSlots;

    /**
     * @brief Renderer2D 的私有资源和当前批次运行状态。
     *
     * 该结构的完整定义只存在于 Renderer2D.cpp，因此公共头文件
     * 不需要包含 Shader、Pipeline、VertexArray 等底层资源头文件。
     *
     * 可以把这里的成员分成三类：
     *
     * 1. 长生命周期 GPU 资源：VAO、VBO、IBO、Shader、Pipeline；
     * 2. 长生命周期 CPU 数据：单位 Quad 顶点、纹理坐标、顶点缓存；
     * 3. 每个场景/批次状态：ViewProjection、写指针、索引数、纹理槽。
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
         * @brief 所有二维 Quad 批次共享的顶点数组对象（VAO）。
         *
         * VAO 不保存实际顶点像素结果。它记录动态 VBO 的属性解析规则，
         * 并记录当前批处理使用的 IBO。Flush() 绑定它后，GPU 才知道
         * 应该怎样解释 QuadVertex 以及按照什么索引顺序组成三角形。
         */
        Scope<VertexArray> QuadVertexArray;

        /**
         * @brief 保存当前批次顶点的动态 GPU VertexBuffer（VBO）。
         *
         * Init() 只申请 MaxVertices 个顶点的空间；DrawQuad() 并不直接
         * 写入该 VBO。Flush() 才通过 SetData() 把 CPU 缓存的有效部分
         * 上传到这里。
         */
        Ref<VertexBuffer> QuadVertexBuffer;

        /**
         * @brief 单位 Quad 的四个局部空间顶点。
         *
         * DrawQuad() 会使用每个 Quad 的 Model 矩阵，
         * 将它们转换为世界空间坐标并写入批处理顶点缓存。
         *
         * 局部范围是 [-0.5, 0.5]，所以单位 Quad 的中心正好位于原点，
         * 缩放和旋转会自然围绕中心发生。
         *
         * 这里使用 vec4，是为了能够直接计算：
         * transform * QuadVertexPositions[i]。
         * w = 1 表示它是位置，会受到平移分量影响。
         */
        std::array<glm::vec4, VerticesPerQuad> QuadVertexPositions{};

        /**
         * @brief 单位 Quad 四个顶点对应的纹理坐标。
         *
         * 顺序必须和 QuadVertexPositions 完全一致：
         * 左下、右下、右上、左上。
         */
        std::array<glm::vec2, VerticesPerQuad> QuadTextureCoordinates{};

        /**
         * @brief CPU 端批处理顶点数组的所有者。
         *
         * 它拥有一块最多容纳 MaxVertices 个 QuadVertex 的连续内存。
         * 每个批次先把所有 Quad 顶点依次写到这里，再把有效区间
         * 一次上传到 GPU VBO。
         *
         * Base 始终指向内存起点并拥有内存，不能随着写入向后移动。
         */
        Scope<QuadVertex[]> QuadVertexBufferBase;

        /**
         * @brief CPU 顶点数组当前的写入位置。
         *
         * 这是非拥有指针，只指向下一处可以写入 QuadVertex 的位置。
         * 每次开始新批次时，它会重新指向 QuadVertexBufferBase 的开头。
         * 每写入一个顶点就向后移动一个 QuadVertex。
         */
        QuadVertex *QuadVertexBufferWritePointer = nullptr;

        /**
         * @brief 当前批次已经写入的索引数量。
         *
         * 每加入一个 Quad 增加 IndicesPerQuad（当前为6）。
         * 达到 MaxIndices 时必须提交当前批次并开始新批次。
         * Flush() 会把它作为 DrawIndexed() 的实际索引数量。
         */
        uint32_t QuadIndexCount = 0;

        /**
         * @brief 保存整个最大批次索引模式的静态 GPU IndexBuffer（IBO）。
         *
         * 第一个 Quad 使用 0,1,2,2,3,0；第二个 Quad 整体加4；
         * 后续 Quad 以相同规律一直预生成到 MaxIndices。
         *
         * 索引规律不会随每帧内容变化，因此 Init() 上传一次后即可复用。
         */
        Ref<IndexBuffer> QuadIndexBuffer;

        /**
         * @brief 纯色 Quad 和纹理 Quad 共用的批处理 Shader。
         *
         * Shader 根据每个顶点的 TextureIndex 决定：
         * 使用 NoTextureIndex 时直接输出 Color，否则从 u_Textures[]
         * 对应槽位采样纹理并与 Color 组合。
         */
        Ref<Shader> TextureShader;


        /**
         * @brief Renderer2D 统一使用的图形管线。
         *
         * 它保存 Shader 以及二维渲染使用的固定功能状态：关闭深度测试、
         * 开启 AlphaBlend、不进行面剔除、使用 TriangleList。
         */
        Ref<GraphicsPipeline> TexturePipeline;

        /**
         * @brief 当前二维场景使用的 ViewProjection 矩阵。
         *
         * 批处理中的顶点已经在 CPU 端变换到了世界空间，
         * Shader 只需要再乘一次 ViewProjection，把世界空间位置变换到
         * 裁剪空间。该矩阵在 BeginScene() 中从 Camera 复制一次。
         */
        glm::mat4 ViewProjection{1.0f};

        /**
         * @brief 当前批次使用的所有二维纹理。
         *
         * 数组下标就是写入 QuadVertex::TextureIndex 的值。
         * Ref 保证纹理在本批次提交完成前不会被销毁。
         * StartBatch() 会清空数组，Flush() 会按照下标逐一绑定。
         */
        std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots{};

        /**
         * @brief 当前批次已经占用的纹理槽数量。
         *
         * 有效纹理槽范围是 [0, TextureSlotCount)。
         * 当数量达到 MaxTextureSlots 时，需要先 Flush() 再开始新批次。
         */
        uint32_t TextureSlotCount = 0;

        /**
         * @brief 当前是否处于 BeginScene() 和 EndScene() 之间。
         *
         * 用于发现重复 BeginScene()、遗漏 EndScene()，以及在场景外
         * 调用 DrawQuad 等生命周期错误。
         */
        bool SceneActive = false;
    };

    /**
     * @brief 定义 Renderer2D 在头文件中声明的静态数据。
     * 外部调用不应该直接访问
     *
     * 此时只创建一个空 Scope，不会创建任何 GPU 资源。
     * 真正的 Renderer2DData 将在 Init() 中创建。
     */
    Scope<Renderer2D::Renderer2DData> Renderer2D::s_Data = nullptr;

    /**
     * @brief 重置当前批次的 CPU 收集状态。
     *
     * StartBatch() 不会重新申请 CPU/GPU 内存，也不会清除屏幕或
     * 已经完成的 Draw Call。它只让接下来的 DrawQuad() 从缓存开头
     * 重新写入，并重新分配本批次纹理槽，因此开销很小。
     */
    void Renderer2D::StartBatch()
    {
        // 所有批处理函数都依赖 Init() 创建的 Renderer2DData。
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::StartBatch requires initialized Renderer2D"
        );

        if (!s_Data)
            return;

        // 新批次还没有任何 Quad，所以实际绘制索引数从0开始。
        s_Data->QuadIndexCount = 0;

        // 从 CPU 顶点数组的第一个位置重新开始写入。
        s_Data->QuadVertexBufferWritePointer = s_Data->QuadVertexBufferBase.get();

        // 释放上一批次保存的纹理 Ref，并清空槽位占用计数。
        // 这不会删除仍被其他 Ref 持有的纹理对象。
        s_Data->TextureSlots.fill(nullptr);
        s_Data->TextureSlotCount = 0;
    }

    /**
     * @brief 把当前 CPU 批次真正提交给 GPU。
     *
     * DrawQuad() 阶段只收集数据，Flush() 才发生真正的 GPU 工作：
     *
     * CPU 顶点缓存 -> 动态 VBO
     * 纹理 Ref 数组 -> GPU 纹理槽
     * ViewProjection -> 当前 Shader Uniform
     * QuadIndexCount -> DrawIndexed 的索引数量
     *
     * 如果当前批次没有 Quad，函数会直接返回，不产生空 Draw Call。
     */
    void Renderer2D::Flush()
    {
        LM_CORE_ASSERT(
            s_Data,
            "Renderer2D::Flush requires initialized Renderer2D"
        );

        if (!s_Data)
            return;

        // 没有索引意味着当前批次没有完整 Quad，无需提交。
        if (s_Data->QuadIndexCount == 0)
            return;

        /*
         * 当前写指针减去数组起始地址，
         * 得到本批次实际写入了多少个 QuadVertex。
         *
         * 指针相减的结果单位是 QuadVertex，而不是字节。例如写入8个
         * 顶点时结果就是8，而不是 8 * sizeof(QuadVertex)。
         */
        const uint32_t vertexCount = static_cast<uint32_t>(
            s_Data->QuadVertexBufferWritePointer
            - s_Data->QuadVertexBufferBase.get()
        );

        // SetData() 的 size 参数以字节为单位，因此还要乘单个顶点大小。
        const uint32_t dataSize =
                static_cast<uint32_t>(
                    vertexCount * sizeof(QuadVertex)
                );

        // 只上传 [Base, WritePointer) 的有效部分，不上传整个最大缓存。
        // offset = 0 表示从动态 VBO 起始位置覆盖本批次数据。
        s_Data->QuadVertexBuffer->SetData(
            s_Data->QuadVertexBufferBase.get(),
            dataSize,
            0
        );

        /*
         * 将当前批次使用的所有纹理绑定到对应纹理槽。
         *
         * TextureSlots 的数组下标必须与 QuadVertex::TextureIndex
         * 完全一致。这样 Fragment Shader 才会从正确的 sampler2D
         * 槽位中取得对应纹理。
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
        // Pipeline::Bind() 负责绑定 Shader，并应用混合、深度、剔除等状态。
        s_Data->TexturePipeline->Bind();

        // 同一批次所有顶点共用当前相机的 ViewProjection 矩阵。
        s_Data->TextureShader->SetMat4(
            "u_ViewProjection",
            s_Data->ViewProjection
        );

        // VAO 告诉 GPU 如何从 QuadVertexBuffer 解析四个属性，
        // 同时提供预生成的 QuadIndexBuffer。
        s_Data->QuadVertexArray->Bind();

        // 只绘制当前批次实际使用的索引，而不是整个 MaxIndices。
        // 一个批次无论包含多少个 Quad，这里都只产生一次 Draw Call。
        RendererCommand::DrawIndexed(
            *s_Data->QuadVertexArray,
            PrimitiveTopology::TriangleList,
            s_Data->QuadIndexCount
        );
    }

    /**
     * @brief 在容量不足时切换到下一个批次。
     *
     * 必须先 Flush()，否则 StartBatch() 重置写指针后会覆盖尚未提交的
     * CPU 数据。已经由 GPU 接收的前一批数据不会因为 StartBatch()
     * 重置 CPU 状态而消失。
     */
    void Renderer2D::NextBatch()
    {
        // 先上传并绘制已经收集的 Quad。
        Flush();

        // 再清空计数器、写指针和纹理槽，收集下一批。
        StartBatch();
    }

    /**
     * @brief 一次性建立 Renderer2D 后续批处理所需的全部资源。
     *
     * 初始化可以按以下顺序理解：
     *
     * 1. 创建 Renderer2DData；
     * 2. 定义单位 Quad 的位置和纹理坐标；
     * 3. 分配 CPU 顶点缓存、GPU VAO 和动态 VBO；
     * 4. 设置 QuadVertex 的属性布局；
     * 5. 预生成整个批次都能复用的静态 IBO；
     * 6. 加载 Shader 并建立 sampler 数组与纹理槽的映射；
     * 7. 创建 Renderer2D 使用的 GraphicsPipeline。
     */
    void Renderer2D::Init()
    {

        // 防止重复初始化覆盖尚未释放的 GPU 资源。
        LM_CORE_ASSERT(!s_Data, "Renderer2D already initialized!");

        if (s_Data)
            return;

        // 从这里开始，s_Data 成为所有 Renderer2D 资源的唯一入口。
        s_Data = CreateScope<Renderer2DData>();

        // 第1步：创建 VAO。VAO 稍后会记录动态 VBO 的布局以及静态 IBO。
        s_Data->QuadVertexArray.reset(VertexArray::Create());

        // 工厂创建可能在 Release 构建中返回 nullptr，所以断言后仍保留保护。
        LM_CORE_ASSERT(s_Data->QuadVertexArray, "Renderer2D: Vertex Array creation failed!");

        if (!s_Data->QuadVertexArray)
        {
            s_Data.reset();
            return;
        }

        /*
         * 第2步：定义以局部原点为中心、宽高均为1的单位 Quad。
         *
         * 顶点顺序为：左下、右下、右上、左上。
         * 该顺序必须与纹理坐标和 IBO 索引生成规则保持一致。
         */
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

        /*
         * 第3步：一次性分配 CPU 批处理缓存。
         *
         * 这里只构造 MaxVertices 个 QuadVertex 的连续内存，
         * 后续 StartBatch() 只重置写指针，不会每帧重复申请。
         */
        s_Data->QuadVertexBufferBase = CreateScope<QuadVertex[]>(MaxVertices);

        // 初始写入位置是 CPU 缓存起点。
        s_Data->QuadVertexBufferWritePointer = s_Data->QuadVertexBufferBase.get();

        /*
         * 创建能够容纳整个最大批次的动态 GPU VBO。
         *
         * 此时只申请容量，没有传入本帧顶点；实际顶点由每次 Flush()
         * 调用 SetData() 更新。
         */
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
         *
         * VertexArray::AddVertexBuffer() 会把这些描述转换成底层 API
         * 的顶点属性设置，并让 Shader location 能正确读取每个字段。
         */
        const VertexBufferLayout quadLayout{
            {ShaderDataType::Float3, "a_Position"},
            {ShaderDataType::Float4, "a_Color"},
            {ShaderDataType::Float2, "a_TexCoord"},
            {ShaderDataType::UInt, "a_TextureIndex"}
        };


        // VBO 保存“它内部一条顶点记录应该怎样解释”的布局描述。
        s_Data->QuadVertexBuffer->SetLayout(quadLayout);

        // VAO 记录该 VBO 及其布局，供 DrawIndexed() 时恢复读取规则。
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

        // 第4步：在 CPU 端创建能够容纳整个批次索引的临时数组。
        // MaxIndices = MaxQuads * IndicesPerQuad。
        Scope<uint32_t[]> quadIndices = CreateScope<uint32_t[]>(MaxIndices);

        // vertexOffset 指向每个 Quad 在批处理顶点数组中的第一个顶点。
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

        // 把预生成索引一次上传到 GPU IBO。
        // IndexBuffer::Create() 的第二个参数是索引数量，不是字节数。
        s_Data->QuadIndexBuffer.reset(IndexBuffer::Create(quadIndices.get(), MaxIndices));


        LM_CORE_ASSERT(s_Data->QuadIndexBuffer, "Renderer2D,Failed to create Renderer2D quad IndexBuffer");

        if (!s_Data->QuadIndexBuffer)
        {
            s_Data.reset();
            return;
        }
        // VAO 同时记录该 IBO，之后绑定 VAO 就能恢复对应索引数据。
        s_Data->QuadVertexArray->SetIndexBuffer(s_Data->QuadIndexBuffer);

        // 恢复干净的 VAO 绑定状态。
        s_Data->QuadVertexArray->UnBind();

        /*
         * 第5步：加载 Renderer2D 的统一 Shader。
         *
         * 纯色和纹理 Quad 使用同一个 Shader，区别由每个顶点携带的
         * TextureIndex 决定，因此它们可以处于同一个批次。
         */
        s_Data->TextureShader = s_Data->Shaders.Load("Renderer2D/Texture2D");

        LM_CORE_ASSERT(s_Data->TextureShader, "Renderer2D batch Shader load failed");

        // Release 构建中断言可能被关闭，因此仍要用运行时分支保护。
        if (!s_Data->TextureShader)
        {
            s_Data.reset();
            return;
        }
        // SetInt() 会修改当前 Shader Program 的 uniform，设置前先绑定。
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
            // 构造 u_Textures[0]、u_Textures[1] ... 这样的 Uniform 名称。
            const std::string uniformName = "u_Textures[" + std::to_string(textureSlot) + "]";

            // Uniform 值就是它需要采样的 GPU texture unit 编号。
            s_Data->TextureShader->SetInt(uniformName.c_str(), textureSlot);
        }

        // 初始化映射结束，解除绑定。
        s_Data->TextureShader->UnBind();

        /*
         * 下面这份 flatColorPipelineSpec 是早期纯色 Pipeline 设计留下的
         * 规格描述。当前纯色和纹理 Quad 已经统一使用 TexturePipeline，
         * 所以这里只保留状态说明，并没有用它创建 GraphicsPipeline。
         */
        GraphicsPipelineSpecification flatColorPipelineSpec;


        flatColorPipelineSpec.Topology = PrimitiveTopology::TriangleList;

        // 当前基础 Renderer2D 按提交顺序绘制，不使用深度缓冲决定遮挡。
        flatColorPipelineSpec.DepthTestEnabled = false;
        flatColorPipelineSpec.DepthWriteEnabled = false;

        // RGBA 颜色需要支持透明度混合
        flatColorPipelineSpec.Blend = BlendMode::AlphaBlend;

        // 2D Quad 不进行正反面剔除，避免负缩放或翻转后消失。
        flatColorPipelineSpec.Culling = CullMode::None;

        // 即使当前不剔除背面，也记录正面的默认逆时针绕序。
        flatColorPipelineSpec.FrontFaceWinding = FrontFace::CounterClockwise;

        // 用于日志、调试和未来的 Pipeline 缓存。
        flatColorPipelineSpec.DebugName = "Renderer2D Flat Color Pipeline";

        /*
         * 第6步：创建统一 Renderer2D 图形管线规格。
         *
         * GraphicsPipelineSpecification 描述“以什么规则绘制”，包括
         * Shader、图元拓扑、深度、混合、剔除和正面绕序。
         */
        GraphicsPipelineSpecification texturePipelineSpecification{};

        texturePipelineSpecification.ShaderProgram =
                s_Data->TextureShader;

        texturePipelineSpecification.Topology =
                PrimitiveTopology::TriangleList;

        // 当前二维对象按提交顺序覆盖，不使用深度缓冲判断前后关系。
        texturePipelineSpecification.DepthTestEnabled = false;
        texturePipelineSpecification.DepthWriteEnabled = false;

        // 使用 Source Alpha 实现普通 RGBA 透明混合。
        texturePipelineSpecification.Blend =
                BlendMode::AlphaBlend;

        // 二维 Quad 两面均可见，也允许负缩放产生的绕序翻转。
        texturePipelineSpecification.Culling =
                CullMode::None;

        texturePipelineSpecification.FrontFaceWinding =
                FrontFace::CounterClockwise;

        texturePipelineSpecification.DebugName =
                "Renderer2D Texture Pipeline";

        // 工厂根据当前 RendererAPI 创建对应后端的 Pipeline 实现。
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

    /**
     * @brief 在图形上下文销毁前释放 Renderer2D 的全部资源。
     *
     * s_Data.reset() 会触发 Renderer2DData 成员的析构；其中 VAO、VBO、
     * IBO、Shader 和 Pipeline 的析构通常需要调用当前图形 API，
     * 所以不能等到 GraphicsContext 已经失效后再执行。
     */
    void Renderer2D::Shutdown()
    {
        // 允许安全地重复调用 Shutdown()，未初始化时无需处理。
        if (!s_Data)
            return;

        LM_CORE_ASSERT(!s_Data->SceneActive,
                       "Renderer2D cannot shut down while a scene is active");

        /*
         * Release 构建中断言可能关闭，因此仍提供兜底处理。
         * 正常使用中不应该依赖该分支，调用者应先显式 EndScene()。
         */
        if (s_Data->SceneActive)
        {
            Renderer::EndScene();
            s_Data->SceneActive = false;
        }

        // 释放 CPU 缓存以及所有由 Renderer2DData 持有的 GPU 资源引用。
        s_Data.reset();
    }

    /**
     * @brief 开始记录一个使用指定 Camera 的二维场景。
     *
     * BeginScene() 不会立即绘制任何 Quad。它只建立本次提交区间的
     * 公共场景状态，并准备一块空的 CPU 批次等待 DrawQuad() 写入。
     */
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
         * Renderer 会保存通用场景数据，例如 ViewProjection 和
         * CameraPosition，供其他渲染路径使用。
         */
        Renderer::BeginScene(camera);

        // Renderer2D 也保存一份矩阵，Flush() 会把它写入批处理 Shader。
        s_Data->ViewProjection = camera.GetViewProjectionMatrix();

        // 清空上一批 CPU 状态，开始收集当前场景的 Quad。
        StartBatch();

        // 所有准备工作完成后才标记场景有效，允许后续 DrawQuad()。
        s_Data->SceneActive = true;
    }

    /**
     * @brief 提交最后一个批次并关闭当前二维场景提交区间。
     *
     * 中途容量不足产生的批次已经由 NextBatch() 提交；这里的 Flush()
     * 负责处理最后一个尚未达到容量上限的批次。
     */
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

        // 通知通用 Renderer：本次场景不再接收新的提交。
        Renderer::EndScene();

        // EndScene() 完成后，场景外调用 DrawQuad() 将触发断言。
        s_Data->SceneActive = false;
    }

    /**
     * @brief 使用中心位置和宽高提交一个纯色 Quad。
     *
     * 这个重载只负责把易用参数转换成 Model 矩阵，真正写入批次的
     * 工作交给 DrawQuad(transform, color)，避免两份重复逻辑。
     */
    void Renderer2D::DrawQuad(const glm::vec3 &position, const glm::vec2 &size, const glm::vec4 &color)
    {
        /*
         * 标准 Quad 以局部原点为中心，初始宽高均为1。
         *
         * Scale 将它缩放为指定宽高；Translate 将缩放后的 Quad
         * 移动到世界空间位置。
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

        // 进入接受完整 Model 矩阵的核心纯色提交函数。
        DrawQuad(
            transform,
            color
        );
    }

    /**
     * @brief 把一个纯色 Quad 的四个顶点写入当前 CPU 批次。
     *
     * 这里不会绑定 Shader、VBO 或 VAO，也不会调用 DrawIndexed()。
     * 它只完成“检查容量 -> 变换四个顶点 -> 写入顶点属性 -> 累加索引数”。
     */
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
            // 取得 CPU 顶点数组中当前可以写入的位置，并将四个字段
            // 分别绑定为局部引用，下面的赋值会直接写入该 QuadVertex。
            auto &[Position, Color, TexCoord, TextureIndex] =
                    *s_Data->QuadVertexBufferWritePointer;

            // 单位 Quad 局部位置经过 Model 矩阵后得到世界空间位置。
            const glm::vec4 &worldPosition = transform * s_Data->QuadVertexPositions[vertexIndex];

            // GPU 顶点格式只需要 xyz，因此丢弃齐次坐标 w。
            Position = glm::vec3(worldPosition);

            // 四个顶点写入相同颜色，光栅化阶段会在三角形内部插值。
            Color = color;
            /*
             * 纯色路径不需要有效纹理坐标。
             * 仍然初始化它们，避免 CPU 缓存中保留无意义旧数据。
             */
            TexCoord = glm::vec2(0.0f);
            /*
             * 该 Quad 是纯色 Quad。
             * Fragment Shader 看到 NoTextureIndex 后不会采样纹理。
             */
            TextureIndex = NoTextureIndex;

            // 指向 CPU 缓存中的下一个空闲 QuadVertex。
            s_Data->QuadVertexBufferWritePointer++;
        }

        // IBO 中每个 Quad 固定使用6个索引。
        s_Data->QuadIndexCount += IndicesPerQuad;
    }

    /**
     * @brief 使用中心位置和宽高提交一个纹理 Quad。
     *
     * 与纯色重载相同，这里只构造 Model 矩阵，然后把核心批处理逻辑
     * 交给 DrawQuad(transform, texture)。
     */
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

        // 进入接受完整 Model 矩阵的核心纹理提交函数。
        DrawQuad(
            transform,
            texture
        );
    }

    /**
     * @brief 把一个纹理 Quad 的四个顶点写入当前 CPU 批次。
     *
     * 除了顶点位置外，该函数还负责当前批次的纹理槽管理：
     *
     * 1. 尝试复用相同 Texture2D 已经占用的槽；
     * 2. 没找到时分配下一个空槽；
     * 3. 槽位已满时先 NextBatch()，再从新批次的槽0开始；
     * 4. 把最终槽下标写入 Quad 四个顶点的 TextureIndex。
     *
     * @param transform 从单位 Quad 局部空间到世界空间的 Model 矩阵。
     * @param texture 当前 Quad 要采样的二维纹理，不能为空。
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
        // 先用“不采样纹理”的保留值表示尚未找到有效纹理槽。
        uint32_t textureIndex = NoTextureIndex;

        // 遍历当前批次已经占用的槽，查找完全相同的 Texture2D Ref。
        for (uint32_t textureSlot = 0;
             textureSlot < s_Data->TextureSlotCount;
             ++textureSlot)
        {
            if (
                s_Data->TextureSlots[textureSlot] ==
                texture
            )
            {
                // 找到后复用槽位，多个 Quad 不会重复占用纹理单元。
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
            /*
             * 使用当前计数指向的下一个空闲槽。
             *
             * 如果刚刚发生 NextBatch()，StartBatch() 已经把
             * TextureSlotCount 重置为0，因此新纹理会进入槽0。
             */
            textureIndex = s_Data->TextureSlotCount;

            // 保存 Ref，保证 Flush() 绑定纹理前对象不会被销毁。
            s_Data->TextureSlots[textureIndex] = texture;

            // 下一次新纹理将使用后面的空槽。
            s_Data->TextureSlotCount++;
        }

        // 把纹理 Quad 的四个顶点连续写入 CPU 批处理缓存。
        for (uint32_t vertexIndex = 0; vertexIndex < VerticesPerQuad; ++vertexIndex)
        {
            // 结构化绑定让每次循环直接填写当前 QuadVertex 的四个字段。
            auto &[Position, Color, TexCoord, TextureIndex]
                    = *s_Data->QuadVertexBufferWritePointer;

            // 单位 Quad 的局部坐标通过 Model 矩阵变换到世界空间。
            const glm::vec4 worldPosition = transform * s_Data->QuadVertexPositions[vertexIndex];

            Position = glm::vec3(worldPosition);

            // 当前没有独立 tint 参数，因此使用白色保持纹理原色。
            // 如果 Shader 计算 textureColor * Color，乘白色不会改变结果。
            Color = glm::vec4(1.0f);

            // 纹理坐标顺序与单位 Quad 顶点顺序一一对应。
            TexCoord = s_Data->QuadTextureCoordinates[vertexIndex];

            // Fragment Shader 根据该值选择 u_Textures[textureIndex]。
            TextureIndex = textureIndex;

            // 移动到 CPU 缓存中的下一个空闲顶点。
            ++s_Data->QuadVertexBufferWritePointer;
        }

        // 当前批次增加一个完整 Quad 对应的6个索引。
        s_Data->QuadIndexCount += IndicesPerQuad;
    }
}
