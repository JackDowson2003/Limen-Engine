//
// Created by chenlong on 2026/10/14.
//
#pragma once
#include <algorithm>
#include <utility>

#include "Render/EditorUIRenderer.h"
#include "Render/EditorSceneContent.h"
#include "Limen/Platform/PlatformPaths.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/Application/Layer.h"
#include "Limen/Core/Log.h"
#include "Limen/Application/Window.h"
#include "Limen/Events/MouseEvent.h"
#include "Limen/Input/Input.h"
#include "Limen/Application/Application.h"
#include "Limen/Scene/SceneRenderer.h"
#include "Limen/Renderer/PerspectiveCameraController.h"
#include "UI/EditorUILayout.h"
#include "Project/EditorSession.h"

namespace LimenEditor
{
    // 编辑器专用 UI Layer：管理当前单窗口占位布局及右侧分隔条的交互状态。
    class EditorUILayer final : public Limen::Layer
    {
    public:
        explicit EditorUILayer(Limen::Ref<EditorSession> session = {})
            : Layer("Limen Editor"), m_Session(std::move(session)),
              m_ViewportCameraController(45.0f, 1.0f, 0.1f, 100.f)
        {
        }

        void OnAttach() override
        {
            if (!m_UIRenderer.Initialize())
                LM_ERROR("Editor UI renderer initialization failed");

            Limen::SceneRendererSpecification viewportSpec;
            viewportSpec.Width = 1;
            viewportSpec.Height = 1;
            viewportSpec.DebugName = "Editor Scene Viewport";
            m_ViewportRenderer = Limen::CreateScope<Limen::SceneRenderer>(viewportSpec);
            m_ViewportCameraController.GetCamera().SetPosition({0.0f, 0.0f, 4.0f});

            // 引擎默认字体：从内置资产根解析路径，失败只记日志、不阻断启动。
            Limen::LString fontError;
            std::filesystem::path fontPath;
            if (Limen::TryGetBuiltInAssetRoot(fontPath, fontError))
            {
                fontPath /= "fonts/NotoSansCJKsc-Regular.otf";
                m_FontAtlas = Limen::CreateScope<Limen::FontAtlas>(1024, 24.0f);
                if (m_FontAtlas->LoadFontFile(fontPath))
                {
                    std::string uiCharacters;
                    for (char c = 32; c < 127; ++c)
                        uiCharacters.push_back(c);
                    uiCharacters += "场景视图对象选择内容浏览器创建详情无";
                    m_FontAtlas->AddText(uiCharacters);
                    if (!m_FontAtlas->Build())
                        LM_ERROR("Editor UI: font atlas build failed");
                } else
                {
                    LM_ERROR("Editor UI: default font load failed: {}",
                             fontPath.string());
                }
            } else
            {
                LM_ERROR("Editor UI: built-in asset root unavailable: {}",
                         fontError);
            }
            if (!m_SceneContent.Initialize())
                LM_ERROR("Editor scene content initialization failed");
        }

        // 每帧把会话场景渲染到与中央区域像素尺寸一致的离屏目标
        void OnUpdate(Limen::DeltaTime &deltaTime) override
        {
            if (!m_Session || !m_ViewportRenderer)
                return;

            const Limen::Window &window = Limen::Application::GetApp().GetWindow();
            const float logicalWidth = static_cast<float>(window.GetWindowWidth());
            const float logicalHeight = static_cast<float>(window.GetWindowHeight());
            if (logicalWidth <= 0.0f || logicalHeight <= 0.0f)
                return;

            const EditorUILayoutResult layout =
                    CalculateEditorUILayout(logicalWidth, logicalHeight, m_RightTopRatio);
            m_CenterRect = layout.Center;

            // 光标在中央区域内，或从视口开始的右键拖拽进行中，才允许相机输入。
            const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
            m_IsViewportHovered = m_CenterRect.Contains(mouseX, mouseY);
            m_ViewportCameraController.SetMouseLookEnabled(
                m_IsViewportHovered || m_IsNavigatingViewport);
            m_ViewportCameraController.OnUpdate(deltaTime);

            const auto [Width, Height] = CalculateViewportPixelSize(
                logicalWidth, logicalHeight, layout.Center,
                window.GetWidth(), window.GetHeight());

            const auto &spec = m_ViewportRenderer->GetSpecification();
            if (spec.Width != Width || spec.Height != Height)
            {
                m_ViewportRenderer->Resize(Width, Height);
                m_ViewportCameraController.GetCamera().SetAspectRatio(
                    static_cast<float>(Width) /
                    static_cast<float>(Height));
            }

            m_ViewportRenderer->Render(
                m_Session->GetScene(),
                m_ViewportCameraController.GetCamera());
        }

        void OnDetach() override
        {
            //保证视口 GPU 资源在 Context 销毁前释放，避免把销毁顺序交给成员析构
            m_ViewportRenderer.reset();
            m_FontAtlas.reset();
            m_UIRenderer.Shutdown();
        }

        // window 是本次绘制目标的非拥有引用；窗口及其图形 Context 由 Application 管理。
        void OnUIRender(Limen::Window &window) override
        {
            // 每次都当前窗口的逻辑尺寸，普通缩放，普通缩放和非全屏最大化走同一套布局
            const float width = static_cast<float>(window.GetWindowWidth());
            const float height = static_cast<float>(window.GetWindowHeight());
            const EditorUILayoutResult &layout = CalculateEditorUILayout(width, height, m_RightTopRatio);
            // 输入命中区域必须与本次实际绘制的分隔条一致。
            m_RightSplitterRect = layout.RightSplitter;
            m_CreateButtonRect = {};

            if (layout.Top.Width <= 0.0f || layout.Top.Height <= 0.0f)
                return;

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Top.X, layout.Top.Y},
                {layout.Top.Width, layout.Top.Height},
                {0.17f, 0.18f, 0.20f, 1.0f}
            );

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Left.X, layout.Left.Y},
                {layout.Left.Width, layout.Left.Height},
                {0.13f, 0.14f, 0.16f, 1.0f}
            );
            // Content Browser 骨架：列出项目资产的逻辑路径，裁剪在左栏内。
            if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_LeftRect = layout.Left;
                const float rowHeight = m_FontAtlas->GetLineHeight() + 4.0f;
                const float textX = layout.Left.X + 12.0f;
                const float headerY = layout.Left.Y + 8.0f;

                m_UIRenderer.SetClipRect(
                    window,
                    {layout.Left.X, layout.Left.Y},
                    {layout.Left.Width, layout.Left.Height});

                m_UIRenderer.DrawText(
                    window,
                    {textX, headerY},
                    {0.85f, 0.85f, 0.85f, 1.0f},
                    *m_FontAtlas,
                    "Content Browser");

                Limen::LArray<Limen::LString> assetRows;
                m_Session->GetAssetRegistry().ForEachRecord(
                    [&](const Limen::AssetRecord &record)
                    {
                        assetRows.push_back(record.LogicalPath);
                    });
                m_AssetRows = assetRows;
                m_AssetFirstRowY = headerY + rowHeight;
                m_AssetRowHeight = rowHeight;

                for (uint32_t row = 0; row < assetRows.getSize(); ++row)
                {
                    m_UIRenderer.DrawText(
                        window,
                        {textX, m_AssetFirstRowY + rowHeight * static_cast<float>(row)},
                        {0.7f, 0.7f, 0.7f, 1.0f},
                        *m_FontAtlas,
                        std::string_view(
                            assetRows[row].data(),
                            assetRows[row].size()));
                }

                m_UIRenderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
            }
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.Center.X, layout.Center.Y},
                {layout.Center.Width, layout.Center.Height},
                {0.08f, 0.09f, 0.11f, 1.0f}
            );
            if (m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_UIRenderer.DrawText(
                    window,
                    {layout.Top.X + 12.0f, layout.Top.Y + 8.0f},
                    {1.0f, 1.0f, 1.0f, 1.0f},
                    *m_FontAtlas,
                    "Limen Editor 场景 视图"
                );
            }

            const std::uintptr_t finalHandle =
                    m_ViewportRenderer
                        ? m_ViewportRenderer->GetFinalColorAttachmentHandle()
                        : 0;
            if (finalHandle != 0)
            {
                m_UIRenderer.DrawNativeTextureRect(
                    window,
                    {layout.Center.X, layout.Center.Y},
                    {layout.Center.Width, layout.Center.Height},
                    finalHandle
                );
            }

            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightTop.X, layout.RightTop.Y},
                {layout.RightTop.Width, layout.RightTop.Height},
                {0.15f, 0.16f, 0.18f, 1.0f}
            );

            // Outliner 骨架：列出场景对象，选中行高亮，裁剪在右上区域。
            if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                m_RightTopRect = layout.RightTop;
                const float rowHeight = m_FontAtlas->GetLineHeight() + 4.0f;
                const float textX = layout.RightTop.X + 12.0f;
                const float headerY = layout.RightTop.Y + 8.0f;

                m_UIRenderer.SetClipRect(
                    window,
                    {layout.RightTop.X, layout.RightTop.Y},
                    {layout.RightTop.Width, layout.RightTop.Height});

                m_UIRenderer.DrawText(
                    window,
                    {textX, headerY},
                    {0.85f, 0.85f, 0.85f, 1.0f},
                    *m_FontAtlas,
                    "Outliner");

                // Outliner 工具栏: Create按钮
                const float buttonWidth = std::min(96.0f, layout.RightTop.Width - 16.0f);

                if (buttonWidth > 0.0f)
                {
                    m_CreateButtonRect = {
                        .X = layout.RightTop.X + layout.RightTop.Width - buttonWidth - 8.0f,
                        .Y = headerY - 2.0f,
                        .Width = buttonWidth,
                        .Height = rowHeight
                    };
                    m_UIRenderer.DrawSolidRect(
                        window,
                        {m_CreateButtonRect.X, m_CreateButtonRect.Y},
                        {m_CreateButtonRect.Width, m_CreateButtonRect.Height},
                        {0.24f, 0.28f, 0.34f, 1.0f}
                    );
                    m_UIRenderer.DrawText(
                        window,
                        {m_CreateButtonRect.X + 8.0f, m_CreateButtonRect.Y + 2.0f},
                        {1.0f, 1.0f, 1.0f, 1.0f},
                        *m_FontAtlas,
                        "Create");
                }

                Limen::LArray<Limen::SceneObjectID> rows;
                m_Session->GetScene().ForEachObject(
                    [&](const Limen::SceneObjectHandle &,
                        const Limen::SceneObjectID &id)
                    {
                        rows.push_back(id);
                    });
                m_OutlinerRows = rows;
                m_OutlinerFirstRowY = headerY + rowHeight;
                m_OutlinerRowHeight = rowHeight;

                for (uint32_t row = 0; row < rows.getSize(); ++row)
                {
                    const Limen::SceneObjectID &id = rows[row];
                    const bool selected = m_Session->IsObjectSelected(id);
                    const glm::vec4 rowColor =
                            selected
                                ? glm::vec4{1.0f, 0.8f, 0.2f, 1.0f}
                                : glm::vec4{0.7f, 0.7f, 0.7f, 1.0f};
                    m_UIRenderer.DrawText(
                        window,
                        {textX, m_OutlinerFirstRowY + rowHeight * static_cast<float>(row)},
                        rowColor,
                        *m_FontAtlas,
                        "对象 " + std::to_string(row + 1));
                }

                m_UIRenderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
            }


            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightSplitter.X, layout.RightSplitter.Y},
                {layout.RightSplitter.Width, layout.RightSplitter.Height},
                {0.35f, 0.37f, 0.42f, 1.0f}
            );
            m_UIRenderer.DrawSolidRect(
                window,
                {layout.RightBottom.X, layout.RightBottom.Y},
                {layout.RightBottom.Width, layout.RightBottom.Height},
                {0.21f, 0.22f, 0.24f, 1.0f}
            );

            // Details 骨架, 显示当前选中对象的平移分量
            if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
            {
                const float textX = layout.RightBottom.X + 12.0f;
                const float headerY = layout.RightBottom.Y + 8.0f;
                const float rowHeight = m_FontAtlas->GetLineHeight() + 4.0f;

                m_UIRenderer.SetClipRect(
                    window,
                    {layout.RightBottom.X, layout.RightBottom.Y},
                    {layout.RightBottom.Width, layout.RightBottom.Height}
                );

                m_UIRenderer.DrawText(
                    window,
                    {textX, headerY},
                    {0.85f, 0.85f, 0.85f, 1.0f},
                    *m_FontAtlas,
                    "Details"
                );
                const Limen::SceneObjectID selectedID = m_Session->GetSelectedObjectID();
                if (!selectedID.isValid())
                {
                    m_UIRenderer.DrawText(
                        window,
                        {textX, headerY + rowHeight},
                        {0.55f, 0.55f, 0.55f, 1.0f},
                        *m_FontAtlas,
                        "No selection"
                    );
                } else
                {
                    // 显示选中的位置
                    glm::mat4 transform{1.0f};
                    if (m_Session->TryGetSelectedObjectTransform(transform))
                    {
                        std::string position = "Pos ";
                        position += std::to_string(transform[3][0]);
                        position += ", ";
                        position += std::to_string(transform[3][1]);
                        position += ", ";
                        position += std::to_string(transform[3][2]);
                        m_UIRenderer.DrawText(
                            window,
                            {textX, headerY + rowHeight},
                            {0.75f, 0.75f, 0.75f, 1.0f},
                            *m_FontAtlas,
                            position);
                    } else
                    {
                        // 不可用，但依然显示
                        m_UIRenderer.DrawText(
                            window,
                            {textX, headerY + rowHeight},
                            {0.8f, 0.4f, 0.4f, 1.0f},
                            *m_FontAtlas,
                            "Transform unavailable"
                        );
                    }
                }
            }
        }

        // event 是 Application 转发给本 Layer 的非拥有事件引用。
        void OnEvent(Limen::Event &event) override
        {
            // 视口内滚轮交给相机控制器：飞行时调速，非飞行时沿视线推拉缩放。
            if (event.GetEventType() == Limen::EventType::MouseScrolled &&
                m_IsViewportHovered)
                m_ViewportCameraController.OnEvent(event);

            // dispatcher 只在本次事件调用期间存在，用于按事件类型调用对应回调。
            Limen::EventDispatcher dispatcher(event);

            // 右键（环视/飞行）或中键（平移）从中央区域按下时，视口接管相机导航。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    const auto button = press.GetButton();
                    if (button != Limen::MouseButton::Right &&
                        button != Limen::MouseButton::Middle)
                        return false;
                    // x、y 是鼠标的逻辑横、纵坐标，用于命中最近一次布局的中央区域。
                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (!m_CenterRect.Contains(x, y))
                        return false;
                    m_IsNavigatingViewport = true;
                    return true;
                });

            dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
                [this](const Limen::MouseButtonReleasedEvent &release)
                {
                    const auto button = release.GetButton();
                    if ((button != Limen::MouseButton::Right &&
                         button != Limen::MouseButton::Middle) ||
                        !m_IsNavigatingViewport)
                        return false;
                    m_IsNavigatingViewport = false;
                    return true;
                });


            // Outliner 工具栏：Create Object 按钮。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;
                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (!m_CreateButtonRect.Contains(x, y))
                        return false;
                    LM_INFO("Editor create button clicked at ({}, {})", x, y);
                    const auto &cubeMesh = m_SceneContent.GetCubeMesh();
                    const auto &cubeMaterial = m_SceneContent.GetCubeMaterial();
                    if (!cubeMesh || !cubeMaterial)
                        return false;
                    const auto created = m_Session->CreateRenderableObject(
                        cubeMesh,
                        cubeMaterial);
                    LM_INFO("CreateRenderableObject valid={}, mesh={}, material={}",
                            created.isValid(),
                            static_cast<bool>(cubeMesh),
                            static_cast<bool>(cubeMaterial));
                    return created.isValid();
                });

            // 左键点击 Outliner 行：把鼠标 Y 映射为行号并选中对应对象。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left ||
                        !m_Session)
                        return false;

                    const auto [x, y] = Limen::Input::GetMousePos();
                    if (!m_RightTopRect.Contains(x, y))
                        return false;

                    const int row = FindListRowAt(
                        m_OutlinerFirstRowY,
                        m_OutlinerRowHeight,
                        y,
                        m_OutlinerRows.getSize());
                    if (row < 0)
                        return false;

                    return m_Session->SelectObject(
                        m_OutlinerRows[static_cast<uint32_t>(row)]);
                });

            // press 仅提供鼠标按钮；按下位置需从当前单窗口的 Input 查询。
            dispatcher.Dispatch<Limen::MouseButtonPressedEvent>(
                [this](const Limen::MouseButtonPressedEvent &press)
                {
                    if (press.GetButton() != Limen::MouseButton::Left)
                        return false;
                    // x、y 分别是鼠标的逻辑横、纵坐标，用于命中最近一次绘制的分隔条。
                    if (
                        const auto [x, y] = Limen::Input::GetMousePos();
                        !m_RightSplitterRect.Contains(x, y)
                    )
                        return false;
                    m_IsDraggingRightSplitter = true;
                    LM_INFO("Editor right splitter grabbed");
                    return true;
                });

            // release 仅提供鼠标按钮；这里只处理从分隔条开始的左键操作。
            dispatcher.Dispatch<Limen::MouseButtonReleasedEvent>(
                [this](const Limen::MouseButtonReleasedEvent &release)
                {
                    if (release.GetButton() != Limen::MouseButton::Left || !m_IsDraggingRightSplitter)
                        return false;
                    // mouseY 是松开时光标的逻辑 Y；ReleasedEvent 本身不携带坐标。
                    const float mouseY = Limen::Input::GetMouseY();
                    // window 是 Application 当前拥有的主窗口，此处只借用它读取最新逻辑高度。
                    const auto &window = Limen::Application::GetApp().GetWindow();
                    // height 是松开时窗口内容区的逻辑高度，不是帧缓冲像素高度。
                    const float width = static_cast<float>(window.GetWindowWidth());
                    const float height = static_cast<float>(window.GetWindowHeight());
                    const EditorUILayoutResult layout = CalculateEditorUILayout(width, height, m_RightTopRatio);

                    // 右侧正文总高：右上区域、分隔条和右下区域都包含在内。
                    const float rightBodyHeight =
                            layout.RightTop.Height
                            + layout.RightSplitter.Height
                            + layout.RightBottom.Height;

                    if (rightBodyHeight > 0.0f)
                    {
                        // 鼠标相对右侧正文顶部的位置，用于决定下一帧的上下比例。
                        const float mouseOffset = mouseY - layout.RightTop.Y;
                        m_RightTopRatio = std::clamp(
                            mouseOffset / rightBodyHeight,
                            0.000001f,
                            0.999999f
                        );
                    }
                    LM_INFO("m_RightTopRatio = {} ", m_RightTopRatio);

                    m_IsDraggingRightSplitter = false;
                    LM_INFO("Editor right splitter released");
                    return true;
                });

            // 尺寸变化后，上一次绘制的逻辑坐标命中区域立即失效；下次 OnUIRender 会重建。
            dispatcher.Dispatch<Limen::WindowResizeEvent>(
                [this](const Limen::WindowResizeEvent &resize)
                {
                    m_RightSplitterRect = {};
                    if (resize.GetWidth() != 0 && resize.GetHeight() != 0)
                        return false;

                    if (m_IsDraggingRightSplitter)
                        LM_INFO("Editor right splitter drag cancelled: framebuffer size is zero");

                    m_IsDraggingRightSplitter = false;

                    // 零尺寸帧缓冲也会打断视口导航；恢复光标，避免右键环视锁定残留。
                    m_IsNavigatingViewport = false;
                    m_IsViewportHovered = false;
                    m_ViewportCameraController.SetMouseLookEnabled(false);
                    return false;
                }
            );
        }

    private:
        //region attributes definitions

        // 本 Layer 按值拥有 UI 绘制器；OnAttach 初始化，OnDetach 在 Context 销毁前关闭。
        EditorUIRenderer m_UIRenderer;
        // 右上面板的绘制比例；绘制时乘以扣除分隔条后的 availableHeight。
        float m_RightTopRatio = 0.5f;
        // 最近一次 OnUIRender 实际绘制的分隔条命中区域；每次绘制前清空并重建，用于鼠标事件的判断
        UIRect m_RightSplitterRect{};

        // 左键是否从分隔条开始按下，等待松开时更新比例；当前并非实时拖动。
        bool m_IsDraggingRightSplitter = false;

        Limen::Ref<EditorSession> m_Session;

        // 视口独占的离屏渲染器与相机；场景来自会话，只借用。
        Limen::Scope<Limen::SceneRenderer> m_ViewportRenderer;
        Limen::PerspectiveCameraController m_ViewportCameraController;

        // 左栏 Content Browser 的命中与行布局，每次绘制重建
        UIRect m_LeftRect{};
        Limen::LArray<Limen::LString> m_AssetRows;
        float m_AssetFirstRowY = 0.0f;
        float m_AssetRowHeight = 0.0f;

        // 右上 Outliner 的命中与行布局，每次绘制重建
        UIRect m_RightTopRect{};
        Limen::LArray<Limen::SceneObjectID> m_OutlinerRows;
        float m_OutlinerFirstRowY = 0.0f;
        float m_OutlinerRowHeight = 0.0f;

        // 最近一帧中央区域；用于判定光标是否在视口内。
        UIRect m_CenterRect{};
        bool m_IsViewportHovered = false;
        bool m_IsNavigatingViewport = false;

        UIRect m_CreateButtonRect{};

        // 引擎默认字体的图集；面板文字只借用，生命周期由本 Layer 管理。
        Limen::Scope<Limen::FontAtlas> m_FontAtlas;

        // Editor 默认立方体等引导内容；GPU 资源由渲染层持有。
        EditorSceneContent m_SceneContent;
        //endregion
    };
}
