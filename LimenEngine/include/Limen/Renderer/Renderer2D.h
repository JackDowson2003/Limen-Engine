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
    class Texture2D;
    class Camera;

    /**
    * @brief 面向客户端的二维渲染器。
    *
    * Renderer2D 将位置、尺寸、颜色和纹理等二维概念，
    * 转换为底层 Renderer 可以理解的 GraphicsPipeline、
    * VertexArray、Transform 和 Draw Call。
    *
    * Renderer2D 不直接调用 OpenGL。真正的图形 API 命令
    * 仍然通过 Renderer、RendererCommand 和 RendererAPI 执行。
    *
    * 当前使用静态设计，整个引擎共享一份 Renderer2D 数据。
    * 这适用于当前单 GraphicsContext、单渲染线程以及场景顺序提交的架构。
    */
    class LIMEN_API Renderer2D
    {
    public:
        Renderer2D() = delete;

        /**
         * @brief 初始化 Renderer2D 使用的公共 GPU 资源。
         *
         * 创建 Quad 的 VAO、VBO、IBO、Shader 和 GraphicsPipeline。
         * 必须在有效的 GraphicsContext 创建之后调用。
         * 初始化高层二维渲染模块需要的资源，例如公共 Quad、2D Shader、UBO 和 Pipeline。
         */
        static void Init();

        /**
         * @brief 释放 Renderer2D 持有的公共 GPU 资源。
         *
         * 必须在 GraphicsContext 被销毁之前调用。(因为需要用到上下文)
         */
        static void Shutdown();

        /**
         * @brief 开始一个二维场景。
         *
         * @param camera
         * 本次二维场景使用的相机。Renderer2D 会从中取得
         * ViewProjectionMatrix。
         */
        static void BeginScene(const Camera &camera);

        /**
        * @brief 结束当前二维场景。
        *
        * 第一版只结束提交区间；以后加入批处理后，
        * 这里还会提交剩余的批次。
        */
        static void EndScene();

        /**
         * @brief 使用纯色绘制一个二维矩形。
         *
         * @param position
         * 矩形中心在世界空间中的位置。
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
        * @brief 根据完整 Model 矩阵绘制纯色 Quad。
        *
        * 当调用者需要旋转、非均匀缩放或者组合变换时，
        * 可以直接传入完整的 Model 矩阵。
        *
        * @param transform
        * Quad 的 Model 矩阵，用于描述平移、旋转和缩放。
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
         * Quad 使用的二维纹理。Ref 允许多个 Quad 共享同一纹理。
         */
        static void DrawQuad(
            const glm::vec3 &position,
            const glm::vec2 &size,
            const Ref<Texture2D> &texture
        );

        /**
         * @brief 根据完整 Model 矩阵绘制纹理 Quad。
         *
         * 当前版本会绑定纹理和纹理 GraphicsPipeline，
         * 然后通过底层 Renderer 提交公共 Quad VAO。
         *
         * @param transform
         * Quad 的 Model 矩阵，用于描述平移、旋转和缩放。
         *
         * @param texture
         * Quad 使用的二维纹理。
         */
        static void DrawQuad(
            const glm::mat4 &transform,
            const Ref<Texture2D> &texture
        );

    private:
        /**
         * @brief Renderer2D 的私有实现数据。
         *
         * 它不是 SceneData 或 ObjectData Uniform 结构，只是集中保存
         * Renderer2D 使用的 VAO、VBO、IBO、Shader、Pipeline 等资源。
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
    };
}
