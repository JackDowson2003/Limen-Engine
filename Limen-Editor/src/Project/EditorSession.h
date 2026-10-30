//
// Created by chenlong on 2026/10/21.
//

#pragma once
#include <utility>

#include "Limen/Asset/AssetRegistry.h"
#include "Limen/Project/ProjectConfiguration.h"
namespace LimenEditor
{
    class EditorSession final
    {
    public:
        explicit EditorSession(Limen::ProjectConfiguration configuration,Limen::Scope<Limen::AssetRegistry> registry)
            : m_Configuration(std::move(configuration)),m_AssetRegistry(std::move(registry))
        {}

        [[nodiscard]]
        const Limen::ProjectConfiguration& GetConfiguration() const noexcept
        {
            return m_Configuration;
        }

        [[nodiscard]]
        const Limen::AssetRegistry& GetAssetRegistry() const noexcept
        {
            return *m_AssetRegistry;
        }
    private:
        // 会话拥有已校验的配置；不借用 CreateApplication 中的局部变量。
        Limen::ProjectConfiguration m_Configuration;
        Limen::Scope<Limen::AssetRegistry> m_AssetRegistry;
    };
}