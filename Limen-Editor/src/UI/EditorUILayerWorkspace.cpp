#include "UI/EditorUILayer.h"

#include <cmath>

#include "Limen/Platform/PlatformPaths.h"
#include "UI/Panels/SidePanelDrawing.h"

namespace LimenEditor
{
    namespace
    {
        void DrawWorkspaceButton(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &font,
            const EditorTheme &theme,
            const UIRect &rect,
            const std::string_view label,
            const bool active = false,
            const bool enabled = true)
        {
            const auto [x, y] = Limen::Input::GetMousePos();
            if (enabled)
            {
                const auto color = rect.Contains(x, y) ? theme.ButtonHoverBackground :
                                   (active ? theme.SelectionBackground : theme.ButtonBackground);
                renderer.DrawRoundedRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height}, color, 5.0f);
            }
            const float textWidth = renderer.MeasureTextWidth(font, label);
            renderer.DrawText(window, {rect.X + (rect.Width - textWidth) * 0.5f,
                                       rect.Y + (rect.Height - font.GetLineHeight()) * 0.5f},
                              enabled ? (active ? theme.SelectionAccent : theme.ButtonText) : theme.TextMuted,
                              font, label);
        }
    }

    void EditorUILayer::LoadWorkspaceFont(const std::string_view sceneLabel)
    {
        std::string workspaceLabels(sceneLabel);
        m_ContentBrowser.AppendVisibleLabels(workspaceLabels);
        if (m_FontAtlas)
        {
            const auto codepoints = m_FontAtlas->DecodeText(workspaceLabels);
            if (std::all_of(codepoints.begin(), codepoints.end(), [this](const char32_t codepoint)
            {
                return m_FontAtlas->FindGlyph(codepoint) != nullptr;
            }))
                return;
        }

        Limen::LString fontError;
        std::filesystem::path fontPath;
        if (!Limen::TryGetBuiltInAssetRoot(fontPath, fontError))
        {
            LM_ERROR("Editor UI: built-in asset root unavailable: {}", fontError);
            return;
        }

        fontPath /= "fonts/NotoSansCJKsc-Regular.otf";
        auto atlas = Limen::CreateScope<Limen::FontAtlas>(1024, m_Theme.FontPixelHeight);
        if (!atlas->LoadFontFile(fontPath))
        {
            LM_ERROR("Editor UI: default font load failed: {}", fontPath.string());
            return;
        }

        std::string uiCharacters;
        for (char c = 32; c < 127; ++c)
            uiCharacters.push_back(c);
        uiCharacters += "场景视图对象选择内容浏览器创建详情无";
        atlas->AddText(uiCharacters);
        if (m_Session)
            atlas->AddText(m_Session->GetConfiguration().DisplayName);
        atlas->AddText(workspaceLabels);
        // 场景或目录切换遇到新字形时重建；构建失败保留原图集。
        if (atlas->Build())
            m_FontAtlas = std::move(atlas);
        else
            LM_ERROR("Editor UI: font atlas build failed");
    }

    EditorUILayoutResult EditorUILayer::CalculateWorkspaceLayout(const Limen::Window &window) const
    {
        return CalculateEditorUILayout(static_cast<float>(window.GetWindowWidth()),
                                       static_cast<float>(window.GetWindowHeight()),
                                       m_ContentHeightRatio, m_Session && m_ContentDrawerOpen, m_ViewportExpanded,
                                       m_DetailsSplitRatio);
    }

    void EditorUILayer::OnUIRender(Limen::Window &window)
    {
        m_WorkspaceLayout = CalculateWorkspaceLayout(window);
        const auto &layout = m_WorkspaceLayout;
        if (layout.Top.Width <= 0.0f || layout.Top.Height <= 0.0f)
        {
            ResetWorkspaceHitAreas();
            return;
        }

        const bool showContent = layout.Content.Width > 0.0f && layout.Content.Height > 0.0f;
        if (m_Session && showContent && m_ContentBrowser.Update(*m_Session))
            LoadWorkspaceFont(m_Session->GetActiveSceneLabel());
        DrawWorkspaceHeader(window, layout.Top);
        const auto fill = [&](const UIRect &rect, const glm::vec4 &color)
        {
            m_UIRenderer.DrawSolidRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height}, color);
        };
        fill(layout.Outliner, m_Theme.PanelBackground);
        fill(layout.Center, m_Theme.ViewportBackground);
        fill(layout.Inspector, m_Theme.PanelBackground);
        fill(layout.Content, m_Theme.PanelBackground);
        m_ViewportPanel.Draw(window, m_UIRenderer, layout.Center);
        DrawViewportToolbar(window, layout.ViewportToolbar);

        if (m_Session && m_FontAtlas && m_FontAtlas->IsBuilt())
        {
            if (layout.Outliner.Width > 0.0f && layout.Outliner.Height > 0.0f)
            {
                m_Outliner.Update(*m_Session);
                m_Outliner.Draw(*m_Session, window, m_UIRenderer, *m_FontAtlas, m_Theme, layout.Outliner);
            }
            if (layout.Inspector.Width > 0.0f && layout.Inspector.Height > 0.0f)
                m_DetailsPanel.Draw(*m_Session, window, m_UIRenderer, *m_FontAtlas, m_Theme, layout.Inspector);
            else
                m_DetailsPanel.ResetHitAreas();
            if (showContent)
                m_ContentBrowser.Draw(*m_Session, window, m_UIRenderer, *m_FontAtlas, m_Theme, layout.Content);
            else
                m_ContentBrowser.ResetLayout();
        }

        if (layout.Outliner.Width > 0.0f)
            fill({layout.Outliner.X, layout.Top.Height, 1.0f, layout.Status.Y - layout.Top.Height},
                 m_Theme.PanelBorder);

        const auto [x, y] = Limen::Input::GetMousePos();
        const bool resizing = m_IsDraggingContentSplitter || layout.ContentSplitter.Contains(x, y);
        fill(layout.ContentSplitter, resizing ? m_Theme.SelectionAccent : m_Theme.Splitter);
        const bool resizingDetails = m_IsDraggingDetailsSplitter || layout.DetailsSplitter.Contains(x, y);
        fill(layout.DetailsSplitter, resizingDetails ? m_Theme.SelectionAccent : m_Theme.Splitter);

        DrawWorkspaceStatusBar(window, layout.Status);
        DrawAssetDragPreview(window);
    }

    void EditorUILayer::DrawWorkspaceHeader(const Limen::Window &window, const UIRect &top)
    {
        m_UIRenderer.DrawSolidRect(window, {top.X, top.Y}, {top.Width, top.Height}, m_Theme.TopBarBackground);
        m_UIRenderer.DrawSolidRect(window, {top.X, top.Y + top.Height - 1.0f},
                                   {top.Width, 1.0f}, m_Theme.PanelBorder);
        if (!m_FontAtlas || !m_FontAtlas->IsBuilt())
            return;

        const auto &font = *m_FontAtlas;
        const bool wide = top.Width >= kEditorWideToolbarWidth;
        const float left = top.X + m_Theme.PanelPadding;
        const float right = top.X + top.Width - m_Theme.PanelPadding;
        const float titleWidth = wide ? std::min(top.Width * 0.32f, 365.0f) : top.Width;
        const float titleHeight = std::min(top.Height, wide ? top.Height : 35.0f);
        const float textY = top.Y + (titleHeight - font.GetLineHeight()) * 0.5f;
        m_UIRenderer.SetClipRect(window, {top.X, top.Y}, {titleWidth, titleHeight});

        constexpr float markSize = 23.0f;
        const float markY = top.Y + (titleHeight - markSize) * 0.5f;
        const float markTextWidth = m_UIRenderer.MeasureTextWidth(font, "L");
        m_UIRenderer.DrawRoundedRect(window, {left, markY}, {markSize, markSize}, m_Theme.Accent, 5.0f);
        m_UIRenderer.DrawText(window, {left + (markSize - markTextWidth) * 0.5f, textY},
                              m_Theme.WindowBackground, font, "L");

        const float nameX = left + markSize + 9.0f;
        m_UIRenderer.DrawText(window, {nameX, textY}, m_Theme.TextPrimary, font, "Limen");
        const float contextX = nameX + m_UIRenderer.MeasureTextWidth(font, "Limen") + 15.0f;
        const float contextWidth = top.X + titleWidth - contextX - 10.0f;
        if (m_Session)
        {
            const std::string_view project = m_Session->GetConfiguration().DisplayName;
            SidePanelDrawing::DrawText(window, m_UIRenderer, font, {contextX, textY},
                                       m_Theme.TextSecondary, contextWidth, project);
        }
        else
        {
            SidePanelDrawing::DrawText(window, m_UIRenderer, font, {contextX, textY},
                                       m_Theme.TextMuted, contextWidth, "No project open");
        }
        m_UIRenderer.SetClipRect(window, {}, {});

        if (!m_Session)
            return;

        constexpr float fileWidth = 309.0f;
        constexpr float groupGap = 15.0f;
        const bool singleCommandRow = wide || right - left >= 540.0f;
        const float rowHeight = wide ? 27.0f : 23.0f;
        const float fileX = wide ? top.X + titleWidth + 4.0f : left;
        const float fileY = wide ? top.Y + (top.Height - rowHeight) * 0.5f : top.Y + 36.0f;
        const float addX = singleCommandRow ? fileX + fileWidth + groupGap : left;
        const float addY = singleCommandRow ? fileY : fileY + rowHeight + 4.0f;
        const float bottom = top.Y + top.Height - 2.0f;
        m_SceneToolbar.Draw(window, m_UIRenderer, font, m_Theme,
            {fileX, fileY, std::min(right - fileX, fileWidth), fileY + rowHeight <= bottom ? rowHeight : 0.0f});
        m_PlacementToolbar.Draw(window, m_UIRenderer, font, m_Theme,
            {addX, addY, right - addX, addY + rowHeight <= bottom ? rowHeight : 0.0f});
    }

    void EditorUILayer::DrawViewportToolbar(const Limen::Window &window, const UIRect &rect)
    {
        m_FrameSelectionRect = {};
        m_ExpandViewportRect = {};
        if (rect.Width <= 0.0f || rect.Height <= 0.0f)
            return;

        m_UIRenderer.DrawSolidRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height}, m_Theme.TopBarBackground);
        m_UIRenderer.DrawSolidRect(window, {rect.X, rect.Y + rect.Height - 1.0f},
                                   {rect.Width, 1.0f}, m_Theme.PanelBorder);
        if (!m_FontAtlas || !m_FontAtlas->IsBuilt())
            return;

        const auto &font = *m_FontAtlas;
        m_UIRenderer.SetClipRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height});
        const float textY = rect.Y + (rect.Height - font.GetLineHeight()) * 0.5f;
        const bool hasControls = m_Session && rect.Width >= 150.0f && rect.Height >= 28.0f;
        const bool hasFrameButton = hasControls && rect.Width >= 243.0f;
        const float reservedWidth = hasFrameButton ? 168.0f : (hasControls ? 88.0f : 0.0f);
        const float tabX = rect.X + 6.0f;
        const float maxTabWidth = std::clamp(rect.Width - reservedWidth - 12.0f, 0.0f, 300.0f);
        const auto label = m_Session ? m_Session->GetActiveSceneLabel() : std::string{"Scene"};
        const float labelWidth = m_UIRenderer.MeasureTextWidth(font, label);
        const float dirtyWidth = m_Session && m_Session->IsDirty() && maxTabWidth >= 42.0f ? 13.0f : 0.0f;
        const float tabWidth = std::min(maxTabWidth, labelWidth + dirtyWidth + 20.0f);
        if (tabWidth >= 20.0f)
        {
            m_UIRenderer.DrawRoundedRect(window, {tabX, rect.Y + 4.0f},
                                         {tabWidth, rect.Height - 5.0f}, m_Theme.PanelBackground, 5.0f);
            m_UIRenderer.DrawSolidRect(window, {tabX + 5.0f, rect.Y + rect.Height - 2.0f},
                                       {tabWidth - 10.0f, 2.0f}, m_Theme.Accent);
            SidePanelDrawing::DrawText(window, m_UIRenderer, font, {tabX + 10.0f, textY},
                                       m_Theme.TextPrimary, tabWidth - dirtyWidth - 20.0f, label);
            if (dirtyWidth > 0.0f)
                m_UIRenderer.DrawText(window, {tabX + tabWidth - 20.0f, textY}, m_Theme.Accent, font, "*");
        }
        const float modeX = tabX + tabWidth + 12.0f;
        const float modeWidth = rect.X + rect.Width - reservedWidth - 10.0f - modeX;
        if (modeWidth >= m_UIRenderer.MeasureTextWidth(font, "Perspective / Lit"))
            m_UIRenderer.DrawText(window, {modeX, textY}, m_Theme.TextMuted, font, "Perspective / Lit");

        if (hasControls)
        {
            constexpr float height = 25.0f;
            const float y = rect.Y + (rect.Height - height) * 0.5f;
            m_ExpandViewportRect = {rect.X + rect.Width - 84.0f, y, 74.0f, height};
            DrawWorkspaceButton(window, m_UIRenderer, font, m_Theme, m_ExpandViewportRect,
                                m_ViewportExpanded ? "Restore" : "Expand", m_ViewportExpanded);
            if (hasFrameButton)
            {
                const bool canFrame = m_Session->GetSelectedObjectCount() > 0;
                const UIRect frame{m_ExpandViewportRect.X - 80.0f, y, 72.0f, height};
                if (canFrame)
                    m_FrameSelectionRect = frame;
                DrawWorkspaceButton(window, m_UIRenderer, font, m_Theme, frame, "Frame", false, canFrame);
            }
        }
        m_UIRenderer.SetClipRect(window, {}, {});
    }

    void EditorUILayer::DrawWorkspaceStatusBar(const Limen::Window &window, const UIRect &rect)
    {
        m_ContentToggleRect = {};
        if (rect.Width <= 0.0f || rect.Height <= 0.0f)
            return;
        m_UIRenderer.DrawSolidRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height}, m_Theme.TopBarBackground);
        m_UIRenderer.DrawSolidRect(window, {rect.X, rect.Y}, {rect.Width, 1.0f}, m_Theme.PanelBorder);
        if (!m_FontAtlas || !m_FontAtlas->IsBuilt())
            return;

        const auto &font = *m_FontAtlas;
        const float left = rect.X + 10.0f;
        const float right = rect.X + rect.Width - 12.0f;
        const float y = rect.Y + (rect.Height - font.GetLineHeight()) * 0.5f;
        m_UIRenderer.SetClipRect(window, {rect.X, rect.Y}, {rect.Width, rect.Height});
        float statusStart = left;
        if (m_Session && rect.Width >= 150.0f && rect.Height >= 24.0f)
        {
            const bool drawerVisible = m_WorkspaceLayout.Content.Height > 0.0f;
            m_ContentToggleRect = {left, rect.Y + 4.0f, 127.0f, rect.Height - 8.0f};
            DrawWorkspaceButton(window, m_UIRenderer, font, m_Theme, m_ContentToggleRect,
                                "Content Drawer", drawerVisible);
            statusStart += m_ContentToggleRect.Width + 13.0f;
        }

        float detailRight = right;
        const auto now = std::chrono::steady_clock::now();
        const auto status = m_StatusMessage.GetText(now);
        if (!status.empty())
        {
            const float width = std::clamp(right - statusStart, 0.0f, m_UIRenderer.MeasureTextWidth(font, status));
            const auto color = m_StatusMessage.GetKind(now) == EditorStatusKind::Error
                ? m_Theme.MissingText : m_Theme.TextSecondary;
            SidePanelDrawing::DrawText(window, m_UIRenderer, font, {right - width, y}, color, width, status);
            detailRight = right - width - 14.0f;
        }
        if (m_Session && rect.Width >= 340.0f)
        {
            const auto selected = m_Session->GetSelectedObjectCount();
            const auto selection = std::to_string(selected) + " selected";
            SidePanelDrawing::DrawText(window, m_UIRenderer, font, {statusStart, y},
                                       m_Theme.TextMuted, detailRight - statusStart, selection);
        }

        m_UIRenderer.SetClipRect(window, {}, {});
    }

    bool EditorUILayer::HandleWorkspaceClick(const float x, const float y)
    {
        if (m_ContentToggleRect.Contains(x, y))
            ToggleContentDrawer();
        else if (m_ExpandViewportRect.Contains(x, y))
            ToggleExpandedViewport();
        else if (m_FrameSelectionRect.Contains(x, y))
        {
            if (!m_ViewportPanel.FrameSelection(*m_Session))
                ShowStatus("Could not frame the selection.", EditorStatusKind::Info);
        }
        else if (m_WorkspaceLayout.ContentSplitter.Contains(x, y))
            m_IsDraggingContentSplitter = true;
        else if (m_WorkspaceLayout.DetailsSplitter.Contains(x, y))
            m_IsDraggingDetailsSplitter = true;
        else
            return false;
        return true;
    }

    bool EditorUILayer::HandleWorkspaceShortcut(const Limen::KeyPressedEvent &key)
    {
        if (key.IsRepeat())
            return false;
        using Limen::Input;
        using Limen::KeyCode;
        const auto code = key.GetKeyCode();
        if (code == KeyCode::Escape && m_ViewportExpanded)
        {
            ToggleExpandedViewport();
            return true;
        }
        if (code == KeyCode::Space &&
            (Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl)))
        {
            ToggleContentDrawer();
            return true;
        }
        if (code == KeyCode::J || code == KeyCode::F)
        {
            const bool command = Input::IsKeyPressed(KeyCode::LeftSuper) || Input::IsKeyPressed(KeyCode::RightSuper) ||
                                 Input::IsKeyPressed(KeyCode::LeftControl) || Input::IsKeyPressed(KeyCode::RightControl);
            if (code == KeyCode::J && command)
            {
                ToggleContentDrawer();
                return true;
            }
            if (code == KeyCode::F && !command)
            {
                const auto [x, y] = Input::GetMousePos();
                if (m_WorkspaceLayout.Center.Contains(x, y))
                    return m_ViewportPanel.FrameSelection(*m_Session);
            }
        }
        return false;
    }

    void EditorUILayer::ResetWorkspaceHitAreas()
    {
        m_WorkspaceLayout = {};
        m_ContentToggleRect = {};
        m_FrameSelectionRect = {};
        m_ExpandViewportRect = {};
        m_ContentBrowser.ResetLayout();
        m_DetailsPanel.ResetHitAreas();
        ResetDragPreview();
    }

    void EditorUILayer::ToggleContentDrawer()
    {
        if (m_ViewportExpanded)
        {
            m_ViewportExpanded = false;
            m_ContentDrawerOpen = true;
        }
        else
        {
            m_ContentDrawerOpen = !m_ContentDrawerOpen;
        }
        m_IsDraggingContentSplitter = false;
        m_IsDraggingDetailsSplitter = false;
        m_ViewportPanel.CancelNavigation();
        ResetWorkspaceHitAreas();
    }

    void EditorUILayer::ToggleExpandedViewport()
    {
        m_ViewportExpanded = !m_ViewportExpanded;
        m_IsDraggingContentSplitter = false;
        m_IsDraggingDetailsSplitter = false;
        m_ViewportPanel.CancelNavigation();
        ResetWorkspaceHitAreas();
    }

    void EditorUILayer::ResizeContentDrawer(const float mouseY)
    {
        const auto layout = CalculateWorkspaceLayout(Limen::Application::GetApp().GetWindow());
        const float splitterHeight = layout.ContentSplitter.Height;
        const float availableHeight = layout.Status.Y - layout.Top.Height - splitterHeight;
        if (splitterHeight > 0.0f && availableHeight > 0.0f && std::isfinite(mouseY))
        {
            // 释放位置决定抽屉顶边，按住期间继续使用已有视口尺寸。
            m_ContentHeightRatio = std::clamp((layout.Status.Y - mouseY - splitterHeight) / availableHeight,
                                              0.0f, 1.0f);
        }
    }

    void EditorUILayer::ResizeDetailsDock(const float mouseY)
    {
        const auto layout = CalculateWorkspaceLayout(Limen::Application::GetApp().GetWindow());
        const float height = layout.Outliner.Height + layout.Inspector.Height;
        if (layout.DetailsSplitter.Height > 0.0f && height > 0.0f && std::isfinite(mouseY))
            m_DetailsSplitRatio = std::clamp((mouseY - layout.Outliner.Y) / height, 0.0f, 1.0f);
    }

    void EditorUILayer::DrawAssetDragPreview(const Limen::Window &window) const
    {
        if (!m_ContentBrowser.IsDraggingAsset() || !m_FontAtlas || !m_FontAtlas->IsBuilt())
            return;

        const auto [x, y] = Limen::Input::GetMousePos();
        Limen::SceneTransform placement;
        if (m_HasDragPreviewBounds && m_ViewportPanel.TryGetPlacementTransformAt(x, y, placement, m_PlacementDistance))
        {
            const auto bounds = m_DragPreviewBounds.Transformed(placement.ToMatrix());
            glm::vec2 low{};
            glm::vec2 high{};
            if (m_ViewportPanel.TryProjectWorldBounds(bounds, low, high))
            {
                auto color = m_Theme.SelectionAccent;
                color.a = 0.8f;
                const auto &view = m_WorkspaceLayout.Center;
                m_UIRenderer.SetClipRect(window, {view.X, view.Y}, {view.Width, view.Height});
                m_UIRenderer.DrawSolidRect(window, low, {high.x - low.x, 1.0f}, color);
                m_UIRenderer.DrawSolidRect(window, {low.x, high.y - 1.0f}, {high.x - low.x, 1.0f}, color);
                m_UIRenderer.DrawSolidRect(window, low, {1.0f, high.y - low.y}, color);
                m_UIRenderer.DrawSolidRect(window, {high.x - 1.0f, low.y}, {1.0f, high.y - low.y}, color);
                m_UIRenderer.SetClipRect(window, {}, {});
            }
        }

        char distance[32]{};
        std::snprintf(distance, sizeof(distance), "  %.1f units", static_cast<double>(m_PlacementDistance));
        std::string label = m_DragPreviewLabel.empty() ? "Place asset" : m_DragPreviewLabel;
        label += distance;
        const auto &font = *m_FontAtlas;
        const float canvasWidth = static_cast<float>(window.GetWindowWidth());
        const float canvasHeight = static_cast<float>(window.GetWindowHeight());
        constexpr float height = 27.0f;
        const float width = std::min(canvasWidth - 12.0f, m_UIRenderer.MeasureTextWidth(font, label) + 18.0f);
        if (width <= 18.0f || canvasHeight < height + 12.0f)
            return;

        const glm::vec2 origin{std::clamp(x + 12.0f, 6.0f, canvasWidth - width - 6.0f),
                               std::clamp(y + 12.0f, 6.0f, canvasHeight - height - 6.0f)};
        m_UIRenderer.DrawRoundedRect(window, origin, {width, height}, m_Theme.ViewportOverlayBackground, 5.0f);
        SidePanelDrawing::DrawText(window, m_UIRenderer, font,
                                   origin + glm::vec2{9.0f, (height - font.GetLineHeight()) * 0.5f},
                                   m_Theme.SelectionText, width - 18.0f, label);
    }
}
