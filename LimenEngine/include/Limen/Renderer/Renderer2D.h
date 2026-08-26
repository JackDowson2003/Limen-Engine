//
// Created by chenlong on 2026/8/25.
//

#pragma once

#include "Limen/Core/Core.h"
#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <glm/vec4.hpp>

namespace Limen
{
    // Renderer2D 的公共接口只保存这两个类型的引用，
    // 因此在头文件中使用前向声明即可，不需要暴露它们的完整定义。
    class Texture2D;
    class Camera;

    /**
     * @brief 面向客户端的二维批处理渲染器。
     *
     * 客户端只需要提交“在哪里、画多大、使用什么颜色或纹理”；
     * Renderer2D 负责把这些高层参数转换成 GPU 可以处理的顶点、
     * 索引、纹理槽、GraphicsPipeline 和 Draw Call。
     *
     * 当前 Renderer2D 的一帧调用顺序是：
     *
     * 1. BeginScene(camera)：保存相机矩阵并开始一个空批次；
     * 2. DrawQuad(...)：把每个 Quad 的四个顶点写入 CPU 顶点缓存；
     * 3. EndScene()：调用 Flush() 上传顶点、绑定纹理并发出 Draw Call。
     *
     * 如果中途达到顶点、索引或纹理槽上限，DrawQuad() 会通过
     * NextBatch() 提前提交当前批次，然后继续收集新的批次。
     *
     * Renderer2D 不直接依赖 OpenGL。最终绘制命令仍然经过
     * GraphicsPipeline、RendererCommand 和 RendererAPI，从而保留
     * 更换底层图形 API 的可能性。
     *
     * 当前采用静态设计，整个引擎共享一份 Renderer2DData。
     * 这适合目前单 GraphicsContext、单渲染线程、场景顺序提交的架构。
     */
    class LIMEN_API Renderer2D
    {
    public:
        // Renderer2D 只提供静态入口，不应该创建 Renderer2D 实例。
        Renderer2D() = delete;

        /**
         * @brief 创建 Renderer2D 在整个运行期间复用的 CPU/GPU 资源。
         *
         * 主要包括 CPU 批处理顶点缓存、VAO、动态 VBO、静态 IBO、
         * Shader、纹理采样器映射以及 GraphicsPipeline。
         *
         * 必须在有效的 GraphicsContext 创建之后调用。
         * 在正常生命周期中只应调用一次，并与 Shutdown() 配对。
         */
        static void Init();

        /**
         * @brief 释放 Renderer2D 持有的公共 GPU 资源。
         *
         * GPU 对象的析构会调用底层图形 API，因此必须在
         * GraphicsContext 被销毁之前调用。
         *
         * Shutdown() 不允许发生在 BeginScene() 与 EndScene() 之间。
         */
        static void Shutdown();

        /**
         * @brief 开始一个二维场景。
         *
         * @param camera
         * 本次二维场景使用的相机。Renderer2D 会从中取得
         * ViewProjectionMatrix，供当前批次中的所有 Quad 共用。
         *
         * 每个 BeginScene() 必须对应一次 EndScene()。
         */
        static void BeginScene(const Camera &camera);

        /**
         * @brief 提交剩余批次并结束当前二维场景。
         *
         * DrawQuad() 只负责向 CPU 缓存写数据；EndScene() 会调用
         * Flush()，把尚未提交的顶点上传到 GPU 并发出 Draw Call，
         * 随后结束底层 Renderer 场景。
         */
        static void EndScene();

        /**
         * @brief 使用纯色绘制一个二维矩形。
         *
         * @param position
         * 矩形中心在世界空间中的位置。position.z 可以用于记录
         * 二维对象的层级，但当前 Pipeline 关闭了深度测试。
         *
         * 当前关闭深度测试，因此重叠顺序主要由 DrawQuad 的提交顺序决定。
         *
         * @param size
         * Quad 在世界空间中的宽度和高度。
         *
         * @param color
         * Quad 的 RGBA 颜色，四个分量通常位于 [0, 1]。
         * color.a 表示透明度：0 为完全透明，1 为完全不透明。
         */
        static void DrawQuad(
            const glm::vec3 &position,
            const glm::vec2 &size,
            const glm::vec4 &color
        );

        /**
         * @brief 根据完整 Model 矩阵提交一个纯色 Quad。
         *
         * 当调用者需要旋转、非均匀缩放或自行组合变换时，
         * 可以使用该重载，避免 Renderer2D 再构造 Model 矩阵。
         *
         * 该函数不会立即发出 Draw Call，而是把变换后的四个顶点
         * 写入当前 CPU 批次。实际提交发生在 Flush()。
         *
         * @param transform
         * 从单位 Quad 局部空间变换到世界空间的 Model 矩阵。
         *
         * @param color
         * Quad 的 RGBA 颜色，四个分量通常位于 [0, 1]。
         * color.a 表示透明度：0 为完全透明，1 为完全不透明。
         */
        static void DrawQuad(
            const glm::mat4 &transform,
            const glm::vec4 &color
        );


        /**
         * @brief 根据位置、尺寸和纹理绘制纹理 Quad。
         *
         * 该重载会在内部生成 Model 矩阵，然后调用接受完整
         * Transform 的纹理 DrawQuad() 重载。
         *
         * @param position
         * Quad 中心在世界空间中的位置。
         *
         * @param size
         * Quad 在世界空间中的宽度和高度。
         *
         * @param texture
         * Quad 使用的二维纹理。Ref 允许多个 Quad 共享同一纹理，
         * 并保证该纹理在批次提交之前仍然有效。
         */
        static void DrawQuad(
            const glm::vec3 &position,
            const glm::vec2 &size,
            const Ref<Texture2D> &texture
        );

        /**
         * @brief 根据完整 Model 矩阵提交一个纹理 Quad。
         *
         * 函数会为纹理查找或分配当前批次中的纹理槽，再把槽下标、
         * 纹理坐标和世界空间位置写入四个批处理顶点。
         *
         * 该函数不会立即绑定纹理或发出 Draw Call；这些操作统一由
         * Flush() 完成，从而让多个 Quad 合并为尽可能少的 Draw Call。
         *
         * @param transform
         * 从单位 Quad 局部空间变换到世界空间的 Model 矩阵。
         *
         * @param texture
         * Quad 使用的二维纹理，不能为空。
         */
        static void DrawQuad(
            const glm::mat4 &transform,
            const Ref<Texture2D> &texture
        );

    private:
        /**
         * @brief Renderer2D 的私有实现数据。
         *
         * 它不是传给 Shader 的 SceneData 或 ObjectData，而是集中保存
         * Renderer2D 的 CPU 批次状态以及 VAO、VBO、IBO、Shader、
         * Pipeline、纹理槽等运行资源。
         *
         * 使用前向声明可以避免把具体资源依赖全部暴露到公共头文件。
         */
        struct Renderer2DData;

        /**
         * @brief 当前引擎共享的一份 Renderer2D 内部数据。
         *
         * Init() 创建该对象，Shutdown() 在 GraphicsContext
         * 销毁之前显式释放它。
         *
         * 使用 Scope 可以明确内部资源的所有权，避免手动 delete。
         */
        static Scope<Renderer2DData> s_Data;

        /**
         * @brief 开始收集一个新的批次。
         *
         * 清空索引数量和纹理槽，并把 CPU 顶点写入指针重置到
         * 顶点缓存开头。它只重置状态，不重新申请内存。
         */
        static void StartBatch();

        /**
         * @brief 提交当前已经收集的批次。
         *
         * 如果当前批次不为空，它会：
         * 1. 计算本批次实际写入的顶点字节数；
         * 2. 将 CPU 顶点上传到动态 VBO；
         * 3. 把本批次纹理绑定到对应纹理槽；
         * 4. 绑定 Pipeline、相机矩阵和 VAO；
         * 5. 使用实际索引数量发出一次 Draw Call。
         */
        static void Flush();

        /**
         * @brief 提交当前批次并开始一个新批次。
         *
         * 当索引容量或纹理槽容量不足时调用。
         * Flush() 保留当前画面结果，StartBatch() 只重置 CPU 端批次状态。
         */
        static void NextBatch();

    };
}
