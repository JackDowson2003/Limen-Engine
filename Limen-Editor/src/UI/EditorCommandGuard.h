#pragma once

#include <chrono>

namespace LimenEditor
{
    // 同一命令在短时间内只执行一次，避免按钮双击产生重复对象。
    class EditorCommandGuard final
    {
    public:
        [[nodiscard]] bool TryAcquire(
            const std::chrono::steady_clock::time_point now) noexcept
        {
            constexpr auto repeatInterval = std::chrono::milliseconds(350);
            if (m_HasTriggered &&
                now >= m_LastTriggerTime &&
                now - m_LastTriggerTime < repeatInterval)
            {
                return false;
            }

            m_LastTriggerTime = now;
            m_HasTriggered = true;
            return true;
        }

        void Reset() noexcept
        {
            m_LastTriggerTime = {};
            m_HasTriggered = false;
        }

    private:
        std::chrono::steady_clock::time_point m_LastTriggerTime{};
        bool m_HasTriggered = false;
    };
}
