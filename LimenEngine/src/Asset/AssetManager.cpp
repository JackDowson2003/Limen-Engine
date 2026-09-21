//
// Created by chenlong on 2026/9/16.
//

#include <string>
#include <unordered_map>
#include <limits>

#include "stb_image/stb_image.h"
#include "Limen/Asset/AssetManager.h"

#include "Limen/Asset/AssetPath.h"
#include "Limen/Asset/ModelImporter.h"
#include "Limen/Core/Log.h"
#include "Limen/RHI/Texture.h"

namespace Limen
{
    namespace
    {
        struct TextureCacheEntry
        {
            Ref<Texture2D> LinearTexture;
            Ref<Texture2D> SRGBTexture;
        };
    }

    /**
     * AssetManager的内部数据。
     *
     * 放在.cpp中，避免把unordered_map等实现细节暴露到公共头文件。
     */
    struct AssetManager::AssetManagerData
    {
        // 实际文件路径 -> 已加载的纹理。
        // 实际文件路径 -> 不同颜色空间的GPU纹理资源。
        std::unordered_map<std::string, TextureCacheEntry> TextureCache;

        // 没有Albedo纹理时使用的1×1白纹理。
        Ref<Texture2D> WhiteTexture;

        // 没有Normal Map时共享的1×1切线空间平坦法线纹理。
        Ref<Texture2D> FlatNormalTexture;

        // 实际文件路径 -> 已加载的模型。
        std::unordered_map<std::string, Ref<Model> > ModelCache;
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

        // RGBA四个通道均为255，表示不透明白色。
        constexpr uint32_t whitePixel = 0xffffffffu;

        s_Data->WhiteTexture->SetData(&whitePixel, sizeof(whitePixel));

        /*
         * 创建1×1 RGBA平坦法线纹理。
         *
         * RGB(128, 128, 255)解码后接近切线空间法线(0, 0, 1)，
         * 可近似保持模型原始法线方向。
         */
        s_Data->FlatNormalTexture = Texture2D::Create(specification);

        LM_CORE_ASSERT(
            s_Data->FlatNormalTexture,
            "Failed to create AssetManager flat normal texture"
        );

        if (!s_Data->FlatNormalTexture)
        {
            s_Data.reset();
            return;
        }

        // 显式写出RGBA字节，避免uint32_t的字节序影响通道顺序。
        constexpr uint8_t flatNormalPixel[4]{128, 128, 255, 255};

        s_Data->FlatNormalTexture->SetData(flatNormalPixel, sizeof(flatNormalPixel));
    }

    void AssetManager::Shutdown()
    {
        if (!s_Data)
            return;
        s_Data.reset();
    }

    Ref<Texture2D> AssetManager::LoadTexture2D(const std::filesystem::path &logicalPath, TextureColorSpace colorSpace)
    {
        // 先将逻辑路径转换成实际文件路径。
        return LoadTexture2DFromFile(
            AssetPath::Resolve(logicalPath),
            colorSpace
        );
    }

    Ref<Model> AssetManager::LoadModel(const std::filesystem::path &logicalPath)
    {
        return LoadModelFromFile(AssetPath::Resolve(logicalPath));
    }

    Ref<Model> AssetManager::LoadModelFromFile(
        const std::filesystem::path &filePath
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
        const std::filesystem::path &filePath,
        const TextureColorSpace colorSpace
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
        if (const auto iterator = s_Data->TextureCache.find(cacheKey);
            iterator != s_Data->TextureCache.end())
        {
            const Ref<Texture2D> &cachedTexture =
                    colorSpace == TextureColorSpace::SRGB
                        ? iterator->second.SRGBTexture
                        : iterator->second.LinearTexture;

            if (cachedTexture)
                return cachedTexture;
        }

        int width = 0;
        int height = 0;
        int channels = 0;

        stbi_set_flip_vertically_on_load(1);

        using StbPixels = std::unique_ptr<stbi_uc, decltype(&stbi_image_free)>;

        const StbPixels pixels(
            stbi_load(
                cacheKey.c_str(),
                &width,
                &height,
                &channels,
                0),
            stbi_image_free
        );
        if (!pixels)
        {
            LM_CORE_WARN(
                "Failed to decode texture '{}': {}",
                cacheKey,
                stbi_failure_reason()
            );
            return nullptr;
        }

        TextureFormat format = TextureFormat::None;

        if (channels == 3)
        {
            format = TextureFormat::RGB8;
        } else if (channels == 4)
        {
            format = TextureFormat::RGBA8;
        } else
        {
            LM_CORE_WARN(
                "Unsupported texture channel count {} for '{}'",
                channels,
                cacheKey
            );
            return nullptr;
        }
        const uint64_t dataSize =
                static_cast<uint64_t>(width) *
                static_cast<uint64_t>(height) *
                static_cast<uint64_t>(channels);

        if (dataSize > std::numeric_limits<uint32_t>::max())
        {
            LM_CORE_WARN(
                "Texture '{}' is too large to upload",
                cacheKey
            );
            return nullptr;
        }

        const Texture2DSpecification specification{
            .Width = static_cast<uint32_t>(width),
            .Height = static_cast<uint32_t>(height),
            .Format = format,
            // 迁移阶段暂时保持Linear，下一步由调用方传入。
            .ColorSpace = colorSpace,
            .GenerateMipmaps = true
        };

        Ref<Texture2D> texture = Texture2D::Create(specification);

        if (!texture)
        {
            LM_CORE_WARN(
                "Failed to create GPU texture '{}'",
                cacheKey
            );
            return nullptr;
        }
        texture->SetData(pixels.get(), static_cast<uint32_t>(dataSize));

        auto &[LinearTexture, SRGBTexture] = s_Data->TextureCache[cacheKey];

        Ref<Texture2D> &cachedTexture =
                colorSpace == TextureColorSpace::SRGB
                    ? SRGBTexture
                    : LinearTexture;

        cachedTexture = texture;

        return texture;
    }

    const Ref<Texture2D> &AssetManager::GetWhiteTexture()
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

    const Ref<Texture2D> &AssetManager::GetFlatNormalTexture()
    {
        LM_CORE_ASSERT(
            s_Data && s_Data->FlatNormalTexture,
            "AssetManager flat normal texture is not available"
        );

        static const Ref<Texture2D> emptyTexture = nullptr;

        if (!s_Data || !s_Data->FlatNormalTexture)
            return emptyTexture;

        return s_Data->FlatNormalTexture;
    }
}
