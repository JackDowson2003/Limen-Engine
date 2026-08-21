#pragma once

#include <cstdint>

namespace Limen
{
    // 引擎自己的键码。不要在这里使用 GLFW、Win32 或 macOS 的原生键码。
    enum class KeyCode : std::uint16_t
    {
        Unknown = 0,

        Space, Apostrophe, Comma, Minus, Period, Slash,
        D0, D1, D2, D3, D4, D5, D6, D7, D8, D9,
        Semicolon, Equal,

        A, B, C, D, E, F, G, H, I, J, K, L, M,
        N, O, P, Q, R, S, T, U, V, W, X, Y, Z,

        LeftBracket, Backslash, RightBracket, GraveAccent,

        Escape, Enter, Tab, Backspace, Delete,
        Right, Left, Down, Up, PageUp, PageDown, Home, End,
        CapsLock, ScrollLock, NumLock, PrintScreen, Pause,

        F1, F2, F3, F4, F5, F6, F7, F8, F9, F10, F11, F12,
        F13, F14, F15, F16, F17, F18, F19, F20, F21, F22, F23, F24, F25,

        // 键盘右侧的数字小键盘（不是顶部的 D0-D9）。
        Keypad0, Keypad1, Keypad2, Keypad3, Keypad4,
        Keypad5, Keypad6, Keypad7, Keypad8, Keypad9,
        KeypadDecimal, KeypadDivide, KeypadMultiply,
        KeypadSubtract, KeypadAdd, KeypadEnter, KeypadEqual,

        LeftShift, LeftControl, LeftAlt, LeftSuper,
        RightShift, RightControl, RightAlt, RightSuper,
        Menu
    };
}
