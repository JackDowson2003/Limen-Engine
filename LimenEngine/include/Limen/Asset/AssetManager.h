//
// Created by chenlong on 2026/9/16.
//

#pragma once

#include <filesystem>

#include "Limen/Core/Core.h"
#include "Limen/RHI/Texture.h"

namespace Limen
{
    class Model;

    /**
     * @brief 统一加载、缓存和管理纹理与模型资源。
     *
     * - 解析逻辑路径；
     * - 缓存同一路径的不同颜色空间纹理和已加载模型；
     * - 提供默认白纹理和平坦法线纹理；
     * - 在 GraphicsContext 销毁前清空管理器持有的资源引用。
     *
     * 当前不是泛型资源系统。
     */
    class LIMEN_API AssetManager final
    {
    public:
        // AssetManager是静态子系统，不允许创建实例。
        AssetManager() = delete;

        /**
         * @brief 初始化资源管理器并创建默认 GPU 纹理。
         *
         * 必须在 GraphicsContext 和 Renderer 初始化之后调用：
         * 此处会创建 1×1 白色 Albedo 纹理和平坦法线纹理。
         *
         * @param assetRoot 当前程序使用的资源根目录。
         */
        static void Init(
            const std::filesystem::path &assetRoot = "assets"
        );

        /**
         * @brief 清空 AssetManager 持有的缓存和默认资源引用。
         *
         * 必须在 GraphicsContext 销毁之前调用。
         * 若调用方仍持有资源 Ref，资源会继续存活；
         * 调用方也必须在 GraphicsContext 销毁前释放这些引用。
         */
        static void Shutdown();

        /**
         * @brief 加载或返回缓存中的Texture2D。
         *
         * @param logicalPath
         * 相对于AssetPath根目录的逻辑路径，
         * 例如"textures/checkerboard.png"。
         *
         * @return
         * 加载成功返回纹理；
         * 文件不存在或解码失败时返回nullptr。
         */
        [[nodiscard]]
        static Ref<Texture2D> LoadTexture2D(
            const std::filesystem::path &logicalPath,
            TextureColorSpace colorSpace
        );

        /**
         * @brief 加载或返回缓存中的Model
         *
         * @param logicalPath 相对于assets根目录的模型路径
         * @return 创建好的Model
         */
        [[nodiscard]] static Ref<Model> LoadModel(const std::filesystem::path &logicalPath);

        [[nodiscard]] static Ref<Model> LoadModelFromFile(const std::filesystem::path &filePath);

        /**
         * @brief 从已经解析好的文件路径加载纹理。
         *
         * 用于OBJ/MTL等资源文件引用的依赖纹理。
         * 该路径已经根据模型文件所在目录完成了解析，
         * 因此不会再次拼接AssetPath根目录。
         *
         * @param filePath 实际文件路径或相对于运行目录的文件路径。
         */
        [[nodiscard]]
        static Ref<Texture2D> LoadTexture2DFromFile(
            const std::filesystem::path &filePath,
            TextureColorSpace colorSpace
        );

        /**
         * @brief 获取所有无Albedo纹理材质共享的默认白纹理。
         */
        [[nodiscard]]
        static const Ref<Texture2D> &GetWhiteTexture();

        /**
         * @brief 获取所有未提供Normal Map的材质共享的默认平坦法线纹理。
         *
         * RGB(128, 128, 255)解码后的切线空间法线接近(0, 0, 1)，
         * 可作为近似保持模型原始法线方向的回退资源。
         */
        [[nodiscard]]
        static const Ref<Texture2D> &GetFlatNormalTexture();

    private:
        /**
         * @brief 隐藏缓存容器和默认资源等实现细节。
         *
         * 公共头文件不需要暴露unordered_map，
         * 后续修改内部缓存结构也不会影响客户端。
         */
        struct AssetManagerData;

        // Init创建，Shutdown显式销毁。
        static Scope<AssetManagerData> s_Data;
    };
}
