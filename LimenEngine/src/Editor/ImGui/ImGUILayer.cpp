//
// Created by chenlong on 2026/8/8.
//
#include "ImGUILayer.h"
#include "imgui.h"
#include "backends/imgui_impl_glfw.h"
#include "backends/imgui_impl_opengl3.h"

#include "glad/gl.h"

#include <GLFW/glfw3.h>
#include "Limen/Application/Application.h"
#include "Limen/Core/Log.h"

namespace Limen
{

    static ImGuiKey ToImGuiKey(const KeyCode keyCode)
    {
        if (keyCode >= KeyCode::D0 && keyCode <= KeyCode::D9)
            return static_cast<ImGuiKey>(ImGuiKey_0 + static_cast<int>(keyCode) - static_cast<int>(KeyCode::D0));

        if (keyCode >= KeyCode::A && keyCode <= KeyCode::Z)
            return static_cast<ImGuiKey>(ImGuiKey_A + static_cast<int>(keyCode) - static_cast<int>(KeyCode::A));

        if (keyCode >= KeyCode::F1 && keyCode <= KeyCode::F24)
            return static_cast<ImGuiKey>(ImGuiKey_F1 + static_cast<int>(keyCode) - static_cast<int>(KeyCode::F1));

        if (keyCode >= KeyCode::Keypad0 && keyCode <= KeyCode::Keypad9)
            return static_cast<ImGuiKey>(ImGuiKey_Keypad0 + static_cast<int>(keyCode) - static_cast<int>(KeyCode::Keypad0));

        switch (keyCode)
        {
            case KeyCode::Space: return ImGuiKey_Space;
            case KeyCode::Apostrophe: return ImGuiKey_Apostrophe;
            case KeyCode::Comma: return ImGuiKey_Comma;
            case KeyCode::Minus: return ImGuiKey_Minus;
            case KeyCode::Period: return ImGuiKey_Period;
            case KeyCode::Slash: return ImGuiKey_Slash;
            case KeyCode::Semicolon: return ImGuiKey_Semicolon;
            case KeyCode::Equal: return ImGuiKey_Equal;
            case KeyCode::LeftBracket: return ImGuiKey_LeftBracket;
            case KeyCode::Backslash: return ImGuiKey_Backslash;
            case KeyCode::RightBracket: return ImGuiKey_RightBracket;
            case KeyCode::GraveAccent: return ImGuiKey_GraveAccent;
            case KeyCode::Escape: return ImGuiKey_Escape;
            case KeyCode::Enter: return ImGuiKey_Enter;
            case KeyCode::Tab: return ImGuiKey_Tab;
            case KeyCode::Backspace: return ImGuiKey_Backspace;
            case KeyCode::Delete: return ImGuiKey_Delete;
            case KeyCode::Right: return ImGuiKey_RightArrow;
            case KeyCode::Left: return ImGuiKey_LeftArrow;
            case KeyCode::Down: return ImGuiKey_DownArrow;
            case KeyCode::Up: return ImGuiKey_UpArrow;
            case KeyCode::PageUp: return ImGuiKey_PageUp;
            case KeyCode::PageDown: return ImGuiKey_PageDown;
            case KeyCode::Home: return ImGuiKey_Home;
            case KeyCode::End: return ImGuiKey_End;
            case KeyCode::CapsLock: return ImGuiKey_CapsLock;
            case KeyCode::ScrollLock: return ImGuiKey_ScrollLock;
            case KeyCode::NumLock: return ImGuiKey_NumLock;
            case KeyCode::PrintScreen: return ImGuiKey_PrintScreen;
            case KeyCode::Pause: return ImGuiKey_Pause;
            case KeyCode::KeypadDecimal: return ImGuiKey_KeypadDecimal;
            case KeyCode::KeypadDivide: return ImGuiKey_KeypadDivide;
            case KeyCode::KeypadMultiply: return ImGuiKey_KeypadMultiply;
            case KeyCode::KeypadSubtract: return ImGuiKey_KeypadSubtract;
            case KeyCode::KeypadAdd: return ImGuiKey_KeypadAdd;
            case KeyCode::KeypadEnter: return ImGuiKey_KeypadEnter;
            case KeyCode::KeypadEqual: return ImGuiKey_KeypadEqual;
            case KeyCode::LeftShift: return ImGuiKey_LeftShift;
            case KeyCode::LeftControl: return ImGuiKey_LeftCtrl;
            case KeyCode::LeftAlt: return ImGuiKey_LeftAlt;
            case KeyCode::LeftSuper: return ImGuiKey_LeftSuper;
            case KeyCode::RightShift: return ImGuiKey_RightShift;
            case KeyCode::RightControl: return ImGuiKey_RightCtrl;
            case KeyCode::RightAlt: return ImGuiKey_RightAlt;
            case KeyCode::RightSuper: return ImGuiKey_RightSuper;
            case KeyCode::Menu: return ImGuiKey_Menu;
            default: return ImGuiKey_None;
        }
    }
    ImGUILayer::ImGUILayer()
        : Layer("ImGUI Layer")
    {
    }


    ImGUILayer::~ImGUILayer() = default;

    void ImGUILayer::OnEvent(Event &e)
    {
        EventDispatcher dispatcher(e);
        //设置各个函数的
        dispatcher.Dispatch<MouseButtonPressedEvent>([this](const MouseButtonPressedEvent &e)
        {
            return OnMouseButtonPressedEvent(e);
        });
        dispatcher.Dispatch<MouseButtonReleasedEvent>([this](const MouseButtonReleasedEvent &e)
        {
            return OnMouseButtonReleaseEvent(e);
        });
        dispatcher.Dispatch<MouseScrolledEvent>([this](const MouseScrolledEvent &e)
        {
            return OnMouseButtonScrolledEvent(e);
        });
        dispatcher.Dispatch<MouseMovedEvent>([this](const MouseMovedEvent &e)
        {
            return OnMouseButtonMovedEvent(e);
        });
        dispatcher.Dispatch<KeyPressedEvent>([this](const KeyPressedEvent &e)
        {
            return OnKeyPressedEvent(e);
        });
        dispatcher.Dispatch<KeyReleasedEvent>([this](const KeyReleasedEvent &e)
        {
            return OnKeyReleasedEvent(e);
        });
        dispatcher.Dispatch<KeyTypedEvent>([this](const KeyTypedEvent &e)
        {
            return OnKeyTyped(e);
        });
    }

    void ImGUILayer::Begin()
    {
        LM_CORE_ASSERT(m_BackendInitialized, "ImGui renderer backend is not initialized");
        if (!m_BackendInitialized)
            return;

        // Renderer backend先准备帧资源，Platform backend再同步窗口和输入。
        switch (m_BackendAPI)
        {
            case RendererAPI::API::OPENGL:
                ImGui_ImplOpenGL3_NewFrame();
                break;
            default:
                LM_CORE_ASSERT(false, "Selected ImGui renderer backend is not implemented");
                return;
        }

        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // 在主视口创建 DockSpace；所有 Layer 的 ImGui 窗口都可停靠于此。
        ImGui::DockSpaceOverViewport(
            ImGui::GetMainViewport()->ID,
            ImGui::GetMainViewport(),
            ImGuiDockNodeFlags_PassthruCentralNode
        );
    }

    void ImGUILayer::End()
    {
        LM_CORE_ASSERT(m_BackendInitialized, "ImGui renderer backend is not initialized");
        if (!m_BackendInitialized)
            return;

        ImGui::Render();

        switch (m_BackendAPI)
        {
            case RendererAPI::API::OPENGL:
                ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
                break;
            default:
                LM_CORE_ASSERT(false, "Selected ImGui renderer backend is not implemented");
                break;
        }
    }

    bool ImGUILayer::OnMouseButtonPressedEvent(const MouseButtonPressedEvent &e)
    {
        ImGuiIO &io = ImGui::GetIO();
        io.AddMouseButtonEvent(e.GetMouseButton(), true);
        return io.WantCaptureMouse;
    }

    bool ImGUILayer::OnMouseButtonReleaseEvent(const MouseButtonReleasedEvent &e)
    {
        ImGui::GetIO().AddMouseButtonEvent(
            e.GetMouseButton(),
            false
        );

        return ImGui::GetIO().WantCaptureMouse;
    }

    bool ImGUILayer::OnMouseButtonScrolledEvent(const MouseScrolledEvent &e)
    {
        ImGui::GetIO().AddMouseWheelEvent(
            e.GetXOffset(),
            e.GetYOffset()
        );

        return ImGui::GetIO().WantCaptureMouse;
    }

    bool ImGUILayer::OnMouseButtonMovedEvent(const MouseMovedEvent &e)
    {
        ImGui::GetIO().AddMousePosEvent(e.GetX(), e.GetY());
        return ImGui::GetIO().WantCaptureMouse;
    }

    bool ImGUILayer::OnKeyPressedEvent(const KeyPressedEvent &e)
    {
        ImGui::GetIO().AddKeyEvent(
            ToImGuiKey(e.GetKeyCode()),
            true
        );

        return ImGui::GetIO().WantCaptureKeyboard;
    }

    bool ImGUILayer::OnKeyReleasedEvent(const KeyReleasedEvent &e)
    {
        ImGui::GetIO().AddKeyEvent(
            ToImGuiKey(e.GetKeyCode()),
            false
        );

        return ImGui::GetIO().WantCaptureKeyboard;
    }

    bool ImGUILayer::OnKeyTyped(const KeyTypedEvent &e)
    {
        ImGui::GetIO().AddInputCharacter(e.GetCodepoint());
        return ImGui::GetIO().WantCaptureKeyboard;
    }

    /**
     * 做好imgui的设置包括glsl_version的设置
     */
    void ImGUILayer::OnAttach()
    {
        IMGUI_CHECKVERSION();
        ///Configuration
        ImGui::CreateContext();
        ImGui::StyleColorsDark();
        ImGuiIO &io = ImGui::GetIO();
        io.BackendFlags |= ImGuiBackendFlags_HasMouseCursors; //Enable MouseCursor
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;      // 允许 App 窗口内的面板停靠。
        io.ConfigFlags &= ~ImGuiConfigFlags_ViewportsEnable;   // 禁止面板拖出 App 后成为原生窗口。
#if GLFW_VERSION_MAJOR >= 3 && GLFW_VERSION_MINOR >= 3
        io.ConfigDpiScaleFonts = true;
        // [Experimental] Automatically overwrite style.FontScaleDpi in Begin() when Monitor DPI changes. This will scale fonts but _NOT_ scale sizes/padding for now.
#endif
        // Setup Platform/Renderer backends
        const auto &window = Application::GetApp().GetWindow();

        auto *glfwWindow = static_cast<GLFWwindow *>(
            window.GetNativeWindow()
        );

        m_BackendAPI = RendererAPI::GetAPI();

        switch (m_BackendAPI)
        {
            case RendererAPI::API::OPENGL:
            {
                ImGui_ImplGlfw_InitForOpenGL(glfwWindow, false);
                constexpr auto glslVersion = "#version 410";
                ImGui_ImplOpenGL3_Init(glslVersion);
                m_BackendInitialized = true;
                LM_CORE_INFO("ImGui renderer backend: OpenGL3");
                break;
            }

            case RendererAPI::API::METAL:
                LM_CORE_ERROR(
                    "Metal was selected, but ImGui_ImplMetal is not integrated yet"
                );
                LM_CORE_ASSERT(false, "Missing ImGui Metal renderer backend");
                break;

            default:
                LM_CORE_ERROR("ImGui does not support the selected RendererAPI yet");
                LM_CORE_ASSERT(false, "Missing ImGui renderer backend");
                break;
        }

#ifdef __EMSCRIPTEN__
        ImGui_ImplGlfw_InstallEmscriptenCallbacks(glfwWindow, "#canvas");
#endif
    }

    void ImGUILayer::OnDetach()
    {
        if (m_BackendInitialized)
        {
            switch (m_BackendAPI)
            {
                case RendererAPI::API::OPENGL:
                    ImGui_ImplOpenGL3_Shutdown();
                    break;
                default:
                    break;
            }

            ImGui_ImplGlfw_Shutdown();
        }

        m_BackendInitialized = false;
        m_BackendAPI = RendererAPI::API::NONE;
        ImGui::DestroyContext();
    }
}
