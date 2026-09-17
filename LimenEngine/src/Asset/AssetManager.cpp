//
// Created by chenlong on 2026/9/16.
//

#include <string>
#include <unordered_map>
#include "Limen/Asset/AssetManager.h"

#include "Limen/Asset/AssetPath.h"
#include "Limen/Asset/ModelImporter.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/Texture.h"

namespace Limen
{
    /**
     * AssetManager的内部数据。
     *
     * 放在.cpp中，避免把unordered_map等实现细节暴露到公共头文件。
     */
    struct AssetManager::AssetManagerData
    {
        // 实际文件路径 -> 已加载的纹理。
        std::unordered_map<std::string, Ref<Texture2D> > TextureCache;

        // 没有Albedo纹理时使用的1×1白纹理。
        Ref<Texture2D> WhiteTexture;

        // model的缓存
        std::unordered_map<std::string, Ref<Model>> ModelCache;
    };


    Scope<AssetManager::AssetManagerData> AssetManager::s_Data = nullptr;

    void AssetManager::Init(const std::filesystem::path &assetRoot)
    {
        LM_CORE_ASSERT(
            !s_Data,
            "AssetManager has already been initialized"
        );

        if (s_Data)
            return;

        AssetPath::SetRoot(assetRoot);

        s_Data = CreateScope<AssetManagerData>();

        /*
         * 创建1×1 RGBA白纹理。
         *
         * 材质没有Albedo纹理时：
         * 白纹理 × 材质颜色 = 材质颜色。
         */
        Texture2DSpecification specification;
        specification.Width = 1;
        specification.Height = 1;
        specification.Format = TextureFormat::RGBA8;
        specification.GenerateMipmaps = false;

        s_Data->WhiteTexture = Texture2D::Create(specification);

        LM_CORE_ASSERT(
            s_Data->WhiteTexture,
            "Failed to create AssetManager white texture"
        );

        if (!s_Data->WhiteTexture)
        {
            s_Data.reset();
            return;
        }

        // RGBA 全f 表示不透明白色 即255
        constexpr uint32_t whitePixel = 0xffffffffu;

        s_Data->WhiteTexture->SetData(&whitePixel, sizeof(whitePixel));
    }

    void AssetManager::Shutdown()
    {
        if (!s_Data)
            return;
        s_Data.reset();
    }

    Ref<Texture2D> AssetManager::LoadTexture2D(const std::filesystem::path &logicalPath)
    {
        // 先将逻辑路径转换成实际文件路径。
        return LoadTexture2DFromFile(
            AssetPath::Resolve(logicalPath)
        );
    }

    Ref<Model> AssetManager::LoadModel(const std::filesystem::path &logicalPath)
    {

        return LoadModelFromFile(AssetPath::Resolve(logicalPath));
    }

    Ref<Model> AssetManager::LoadModelFromFile(
    const std::filesystem::path& filePath
)
    {
        if (!s_Data || filePath.empty())
            return nullptr;

        const std::filesystem::path normalizedPath =
                filePath.lexically_normal();

        const std::string key =
                filePath.lexically_normal().generic_string();

        if (const auto iterator = s_Data->ModelCache.find(key); iterator != s_Data->ModelCache.end())
        {
            return iterator->second;
        }

        // 绝对路径和有效的相对路径都可以交给Importer。
        Ref<Model> model = ModelImporter::Import(normalizedPath);

        if (!model)
            return nullptr;

        s_Data->ModelCache.emplace(key, model);

        return model;
    }

    Ref<Texture2D> AssetManager::LoadTexture2DFromFile(
        const std::filesystem::path& filePath
    )
    {
        LM_CORE_ASSERT(
            s_Data,
            "AssetManager must be initialized before loading textures"
        );

        if (!s_Data)
            return nullptr;

        if (filePath.empty())
        {
            LM_CORE_WARN("Cannot load texture from an empty path");
            return nullptr;
        }

        const std::string cacheKey = filePath.lexically_normal().generic_string();

        // 已经加载过时直接复用原纹理。
        if (const auto iterator =
                s_Data->TextureCache.find(cacheKey);
            iterator != s_Data->TextureCache.end())
        {
            return iterator->second;
        }

        Ref<Texture2D> texture =
                Texture2D::Create(cacheKey.c_str());

        if (!texture)
        {
            LM_CORE_WARN(
                "Failed to load texture '{}'",
                cacheKey
            );

            return nullptr;
        }

        s_Data->TextureCache.emplace(cacheKey, texture);

        return texture;
    }

    const Ref<Texture2D>& AssetManager::GetWhiteTexture()
    {
        LM_CORE_ASSERT(
            s_Data && s_Data->WhiteTexture,
            "AssetManager white texture is not available"
        );

        // 函数返回引用，不能直接return nullptr，
        // 否则会返回临时shared_ptr的悬空引用。
        static const Ref<Texture2D> emptyTexture = nullptr;

        if (!s_Data || !s_Data->WhiteTexture)
            return emptyTexture;

        return s_Data->WhiteTexture;
    }
}
