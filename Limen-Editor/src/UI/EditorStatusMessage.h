#pragma once

#include <chrono>
#include <string>
#include <string_view>
#include <utility>

namespace LimenEditor
{
    enum class EditorStatusKind
    {
        Info,
        Error
    };

    // Editor 顶部临时状态消息；查询时自动判断是否过期。
    class EditorStatusMessage final
    {
    public:
        void Show(
            std::string text,
            const EditorStatusKind kind,
            const std::chrono::steady_clock::time_point now,
            const std::chrono::steady_clock::duration duration =
                std::chrono::seconds(10))
        {
            if (text.empty() || duration <= std::chrono::steady_clock::duration::zero())
            {
                Clear();
                return;
            }

            m_Text = std::move(text);
            m_Kind = kind;
            m_ExpiryTime = now + duration;
        }

        void Clear() noexcept
        {
            m_Text.clear();
            m_Kind = EditorStatusKind::Info;
            m_ExpiryTime = {};
        }

        [[nodiscard]] std::string_view GetText(
            const std::chrono::steady_clock::time_point now) const noexcept
        {
            if (m_Text.empty() || now >= m_ExpiryTime)
                return {};
            return m_Text;
        }

        [[nodiscard]] EditorStatusKind GetKind(
            const std::chrono::steady_clock::time_point now) const noexcept
        {
            if (m_Text.empty() || now >= m_ExpiryTime)
                return EditorStatusKind::Info;
            return m_Kind;
        }

    private:
        std::string m_Text;
        EditorStatusKind m_Kind = EditorStatusKind::Info;
        std::chrono::steady_clock::time_point m_ExpiryTime{};
    };
}
