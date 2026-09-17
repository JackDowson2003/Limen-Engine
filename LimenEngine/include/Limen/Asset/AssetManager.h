//
// Created by chenlong on 2026/9/16.
//

#pragma once

#include <filesystem>

#include "Limen/Core/Core.h"

namespace Limen
{
    class Texture2D;

    class Model;

    /**
     * @brief 统一加载、缓存和管理引擎资源
     *
     * 第一版只管理 Textrue2D
     *
     * - 逻辑路径解析；
     * - 避免相同纹理重复加载；
     * - 提供默认白纹理；
     * - 在GraphicsContext销毁前统一释放GPU资源。
     *
     * 后续再加入Model缓存，不提前做泛型资源系统。
     */
    class LIMEN_API AssetManager final
    {
    public:
        // AssetManager是静态子系统，不允许创建实例。
        AssetManager() = delete;

        /**
         * @brief 初始化资源管理器。
         *
         * 必须在GraphicsContext和Renderer初始化之后调用，
         * 因为初始化过程会创建默认白色GPU纹理。
         *
         * @param assetRoot
         * 当前程序使用的资源根目录。
         */
        static void Init(
            const std::filesystem::path& assetRoot = "assets"
        );

        /**
         * @brief 释放缓存中的全部资源。
         *
         * 必须在GraphicsContext销毁之前调用，
         * 否则Texture析构时无法安全调用底层图形API。
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
        static Ref<Texture2D> LoadTexture2D(const std::filesystem::path& logicalPath);

        /**
         * @brief 加载或返回缓存中的Model
         *
         * @param logicalPath 相对于assets根目录的模型路径
         * @return 创建好的Model
         */
        [[nodiscard]] static Ref<Model> LoadModel(const std::filesystem::path& logicalPath);

        [[nodiscard]] static Ref<Model> LoadModelFromFile(const std::filesystem::path& filePath);

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
            const std::filesystem::path& filePath
        );

        /**
         * @brief 获取所有无Albedo纹理材质共享的默认白纹理。
         */
        [[nodiscard]]
        static const Ref<Texture2D>& GetWhiteTexture();

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