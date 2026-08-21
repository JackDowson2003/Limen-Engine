#pragma once

#include <sstream>

#include "Limen/Events/Event.h"
#include "Limen/Input/KeyCodes.h"

namespace Limen
{
    class LIMEN_API KeyEvent : public Event
    {
    public:
        [[nodiscard]] KeyCode GetKeyCode() const { return m_KeyCode; }

        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    protected:
        explicit KeyEvent(const KeyCode keyCode)
            : m_KeyCode(keyCode) {}

        KeyCode m_KeyCode;
    };

    class LIMEN_API KeyPressedEvent : public KeyEvent
    {
    public:
        KeyPressedEvent(const KeyCode keyCode, const int repeatCount = 0)
            : KeyEvent(keyCode), m_RepeatCount(repeatCount) {}

        [[nodiscard]] bool IsRepeat() const { return m_RepeatCount > 0; }

        [[nodiscard]] std::string ToString() const override
        {
            std::stringstream stream;
            stream << "KeyPressedEvent: " << static_cast<int>(m_KeyCode)
                   << " (repeatCount = " << m_RepeatCount << ')';
            return stream.str();
        }

        EVENT_CLASS_TYPE(KeyPressed)

    private:
        int m_RepeatCount;
    };

    class LIMEN_API KeyReleasedEvent : public KeyEvent
    {
    public:
        explicit KeyReleasedEvent(const KeyCode keyCode)
            : KeyEvent(keyCode) {}

        [[nodiscard]] std::string ToString() const override
        {
            std::stringstream stream;
            stream << "KeyReleasedEvent: " << static_cast<int>(m_KeyCode);
            return stream.str();
        }

        EVENT_CLASS_TYPE(KeyReleased)
    };

    // 文本输入事件：保存的是 Unicode 码点，而不是某个物理按键。
    class LIMEN_API KeyTypedEvent : public Event
    {
    public:
        explicit KeyTypedEvent(const char32_t codepoint)
            : m_Codepoint(codepoint) {}

        [[nodiscard]] char32_t GetCodepoint() const { return m_Codepoint; }

        [[nodiscard]] std::string ToString() const override
        {
            std::stringstream stream;
            stream << "KeyTypedEvent: " << static_cast<std::uint32_t>(m_Codepoint);
            return stream.str();
        }

        EVENT_CLASS_TYPE(KeyTyped)
        EVENT_CLASS_CATEGORY(EventCategoryKeyboard | EventCategoryInput)

    private:
        char32_t m_Codepoint;
    };
}
