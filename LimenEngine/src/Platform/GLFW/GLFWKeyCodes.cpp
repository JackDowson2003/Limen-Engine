#include "GLFWKeyCodes.h"

#include <GLFW/glfw3.h>

namespace Limen
{

    KeyCode KeyCodeFromGLFW(const int glfwKey)
    {
        if (glfwKey >= GLFW_KEY_0 && glfwKey <= GLFW_KEY_9)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::D0) + glfwKey - GLFW_KEY_0);

        if (glfwKey >= GLFW_KEY_A && glfwKey <= GLFW_KEY_Z)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::A) + glfwKey - GLFW_KEY_A);

        if (glfwKey >= GLFW_KEY_F1 && glfwKey <= GLFW_KEY_F25)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::F1) + glfwKey - GLFW_KEY_F1);

        if (glfwKey >= GLFW_KEY_KP_0 && glfwKey <= GLFW_KEY_KP_9)
            return static_cast<KeyCode>(static_cast<int>(KeyCode::Keypad0) + glfwKey - GLFW_KEY_KP_0);

        switch (glfwKey)
        {
            case GLFW_KEY_SPACE: return KeyCode::Space;
            case GLFW_KEY_APOSTROPHE: return KeyCode::Apostrophe;
            case GLFW_KEY_COMMA: return KeyCode::Comma;
            case GLFW_KEY_MINUS: return KeyCode::Minus;
            case GLFW_KEY_PERIOD: return KeyCode::Period;
            case GLFW_KEY_SLASH: return KeyCode::Slash;
            case GLFW_KEY_SEMICOLON: return KeyCode::Semicolon;
            case GLFW_KEY_EQUAL: return KeyCode::Equal;
            case GLFW_KEY_LEFT_BRACKET: return KeyCode::LeftBracket;
            case GLFW_KEY_BACKSLASH: return KeyCode::Backslash;
            case GLFW_KEY_RIGHT_BRACKET: return KeyCode::RightBracket;
            case GLFW_KEY_GRAVE_ACCENT: return KeyCode::GraveAccent;
            case GLFW_KEY_ESCAPE: return KeyCode::Escape;
            case GLFW_KEY_ENTER: return KeyCode::Enter;
            case GLFW_KEY_TAB: return KeyCode::Tab;
            case GLFW_KEY_BACKSPACE: return KeyCode::Backspace;
            case GLFW_KEY_DELETE: return KeyCode::Delete;
            case GLFW_KEY_RIGHT: return KeyCode::Right;
            case GLFW_KEY_LEFT: return KeyCode::Left;
            case GLFW_KEY_DOWN: return KeyCode::Down;
            case GLFW_KEY_UP: return KeyCode::Up;
            case GLFW_KEY_PAGE_UP: return KeyCode::PageUp;
            case GLFW_KEY_PAGE_DOWN: return KeyCode::PageDown;
            case GLFW_KEY_HOME: return KeyCode::Home;
            case GLFW_KEY_END: return KeyCode::End;
            case GLFW_KEY_CAPS_LOCK: return KeyCode::CapsLock;
            case GLFW_KEY_SCROLL_LOCK: return KeyCode::ScrollLock;
            case GLFW_KEY_NUM_LOCK: return KeyCode::NumLock;
            case GLFW_KEY_PRINT_SCREEN: return KeyCode::PrintScreen;
            case GLFW_KEY_PAUSE: return KeyCode::Pause;
            case GLFW_KEY_KP_DECIMAL: return KeyCode::KeypadDecimal;
            case GLFW_KEY_KP_DIVIDE: return KeyCode::KeypadDivide;
            case GLFW_KEY_KP_MULTIPLY: return KeyCode::KeypadMultiply;
            case GLFW_KEY_KP_SUBTRACT: return KeyCode::KeypadSubtract;
            case GLFW_KEY_KP_ADD: return KeyCode::KeypadAdd;
            case GLFW_KEY_KP_ENTER: return KeyCode::KeypadEnter;
            case GLFW_KEY_KP_EQUAL: return KeyCode::KeypadEqual;
            case GLFW_KEY_LEFT_SHIFT: return KeyCode::LeftShift;
            case GLFW_KEY_LEFT_CONTROL: return KeyCode::LeftControl;
            case GLFW_KEY_LEFT_ALT: return KeyCode::LeftAlt;
            case GLFW_KEY_LEFT_SUPER: return KeyCode::LeftSuper;
            case GLFW_KEY_RIGHT_SHIFT: return KeyCode::RightShift;
            case GLFW_KEY_RIGHT_CONTROL: return KeyCode::RightControl;
            case GLFW_KEY_RIGHT_ALT: return KeyCode::RightAlt;
            case GLFW_KEY_RIGHT_SUPER: return KeyCode::RightSuper;
            case GLFW_KEY_MENU: return KeyCode::Menu;
            default: return KeyCode::Unknown;
        }
    }

    int GLFWKeyFromKeyCode(const KeyCode keyCode)
    {
        if (keyCode >= KeyCode::D0 && keyCode <= KeyCode::D9)
            return GLFW_KEY_0 + static_cast<int>(keyCode) - static_cast<int>(KeyCode::D0);

        if (keyCode >= KeyCode::A && keyCode <= KeyCode::Z)
            return GLFW_KEY_A + static_cast<int>(keyCode) - static_cast<int>(KeyCode::A);

        if (keyCode >= KeyCode::F1 && keyCode <= KeyCode::F25)
            return GLFW_KEY_F1 + static_cast<int>(keyCode) - static_cast<int>(KeyCode::F1);

        if (keyCode >= KeyCode::Keypad0 && keyCode <= KeyCode::Keypad9)
            return GLFW_KEY_KP_0 + static_cast<int>(keyCode) - static_cast<int>(KeyCode::Keypad0);

        switch (keyCode)
        {
            case KeyCode::Space: return GLFW_KEY_SPACE;
            case KeyCode::Apostrophe: return GLFW_KEY_APOSTROPHE;
            case KeyCode::Comma: return GLFW_KEY_COMMA;
            case KeyCode::Minus: return GLFW_KEY_MINUS;
            case KeyCode::Period: return GLFW_KEY_PERIOD;
            case KeyCode::Slash: return GLFW_KEY_SLASH;
            case KeyCode::Semicolon: return GLFW_KEY_SEMICOLON;
            case KeyCode::Equal: return GLFW_KEY_EQUAL;
            case KeyCode::LeftBracket: return GLFW_KEY_LEFT_BRACKET;
            case KeyCode::Backslash: return GLFW_KEY_BACKSLASH;
            case KeyCode::RightBracket: return GLFW_KEY_RIGHT_BRACKET;
            case KeyCode::GraveAccent: return GLFW_KEY_GRAVE_ACCENT;
            case KeyCode::Escape: return GLFW_KEY_ESCAPE;
            case KeyCode::Enter: return GLFW_KEY_ENTER;
            case KeyCode::Tab: return GLFW_KEY_TAB;
            case KeyCode::Backspace: return GLFW_KEY_BACKSPACE;
            case KeyCode::Delete: return GLFW_KEY_DELETE;
            case KeyCode::Right: return GLFW_KEY_RIGHT;
            case KeyCode::Left: return GLFW_KEY_LEFT;
            case KeyCode::Down: return GLFW_KEY_DOWN;
            case KeyCode::Up: return GLFW_KEY_UP;
            case KeyCode::PageUp: return GLFW_KEY_PAGE_UP;
            case KeyCode::PageDown: return GLFW_KEY_PAGE_DOWN;
            case KeyCode::Home: return GLFW_KEY_HOME;
            case KeyCode::End: return GLFW_KEY_END;
            case KeyCode::CapsLock: return GLFW_KEY_CAPS_LOCK;
            case KeyCode::ScrollLock: return GLFW_KEY_SCROLL_LOCK;
            case KeyCode::NumLock: return GLFW_KEY_NUM_LOCK;
            case KeyCode::PrintScreen: return GLFW_KEY_PRINT_SCREEN;
            case KeyCode::Pause: return GLFW_KEY_PAUSE;
            case KeyCode::KeypadDecimal: return GLFW_KEY_KP_DECIMAL;
            case KeyCode::KeypadDivide: return GLFW_KEY_KP_DIVIDE;
            case KeyCode::KeypadMultiply: return GLFW_KEY_KP_MULTIPLY;
            case KeyCode::KeypadSubtract: return GLFW_KEY_KP_SUBTRACT;
            case KeyCode::KeypadAdd: return GLFW_KEY_KP_ADD;
            case KeyCode::KeypadEnter: return GLFW_KEY_KP_ENTER;
            case KeyCode::KeypadEqual: return GLFW_KEY_KP_EQUAL;
            case KeyCode::LeftShift: return GLFW_KEY_LEFT_SHIFT;
            case KeyCode::LeftControl: return GLFW_KEY_LEFT_CONTROL;
            case KeyCode::LeftAlt: return GLFW_KEY_LEFT_ALT;
            case KeyCode::LeftSuper: return GLFW_KEY_LEFT_SUPER;
            case KeyCode::RightShift: return GLFW_KEY_RIGHT_SHIFT;
            case KeyCode::RightControl: return GLFW_KEY_RIGHT_CONTROL;
            case KeyCode::RightAlt: return GLFW_KEY_RIGHT_ALT;
            case KeyCode::RightSuper: return GLFW_KEY_RIGHT_SUPER;
            case KeyCode::Menu: return GLFW_KEY_MENU;
            default: return GLFW_KEY_UNKNOWN;
        }
    }

}
