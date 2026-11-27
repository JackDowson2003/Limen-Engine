//
// Created by chenlong on 2026/11/21.
//
#include "UI/Panels/DetailsPanel.h"

#include <algorithm>
#include <iomanip>
#include <limits>
#include <sstream>
#include <string_view>
#include <variant>

#include <glm/trigonometric.hpp>

#include "Limen/Application/Window.h"
#include "Limen/Input/Input.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/Scene/SceneComponent.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/Panels/DetailsComponentText.h"
#include "UI/EditorTheme.h"
#include "UI/Panels/SidePanelDrawing.h"

namespace LimenEditor
{
    namespace
    {
        std::string FormatFloat(const float value)
        {
            std::ostringstream output;
            output << std::fixed << std::setprecision(2) << value;
            return output.str();
        }

        std::string FormatVector(const glm::vec3 &value)
        {
            return FormatFloat(value.x) + ", " + FormatFloat(value.y) + ", " + FormatFloat(value.z);
        }

        std::string EditValue(const glm::vec3 &value)
        {
            std::ostringstream output;
            output.imbue(std::locale::classic());
            output << std::setprecision(std::numeric_limits<float>::max_digits10)
                   << value.x << ", " << value.y << ", " << value.z;
            return output.str();
        }

        float DrawSectionHeader(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect,
            const float y,
            const std::string_view label)
        {
            const float lineHeight = fontAtlas.GetLineHeight();
            const float left = rect.X + theme.PanelPadding;
            const float contentWidth = rect.Width - theme.PanelPadding * 2.0f;
            const float headingWidth = renderer.MeasureTextWidth(fontAtlas, label) + 12.0f;
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, y},
                                       theme.TextSecondary, contentWidth, label);
            if (contentWidth > headingWidth)
                renderer.DrawSolidRect(window, {left + headingWidth, y + lineHeight * 0.5f},
                                       {contentWidth - headingWidth, 1.0f}, theme.PanelBorder);
            return y + lineHeight + 7.0f;
        }

        float DrawPropertyRow(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect,
            const float y,
            const std::string_view name,
            const std::string_view value,
            UIRect *editable = nullptr,
            const bool focused = false,
            const bool invalid = false)
        {
            const float lineHeight = fontAtlas.GetLineHeight();
            const float left = rect.X + theme.PanelPadding;
            const float contentWidth = rect.Width - theme.PanelPadding * 2.0f;
            // 栏宽不足时把值放到下一行，保持两部分都能阅读。
            const bool stack = contentWidth < 144.0f;
            const float labelWidth = stack ? contentWidth : std::min(78.0f, contentWidth * 0.36f);
            const float valueOffset = stack ? 0.0f : labelWidth + 10.0f;
            const float valueY = y + (stack ? lineHeight + 2.0f : 0.0f);
            float fieldPadding = 0.0f;
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, y},
                                       theme.TextMuted, labelWidth, name);
            if (editable && contentWidth - valueOffset > 24.0f &&
                valueY + lineHeight + 3.0f <= rect.Y + rect.Height)
            {
                *editable = {left + valueOffset, valueY - 3.0f, contentWidth - valueOffset, lineHeight + 6.0f};
                const auto [mouseX, mouseY] = Limen::Input::GetMousePos();
                const auto background = focused ? theme.SelectionBackground
                    : (editable->Contains(mouseX, mouseY) ? theme.ButtonHoverBackground : theme.TopBarBackground);
                renderer.DrawRoundedRect(window, {editable->X, editable->Y},
                                         {editable->Width, editable->Height}, background, 4.0f);
                fieldPadding = 7.0f;
            }
            SidePanelDrawing::DrawText(window, renderer, fontAtlas,
                                       {left + valueOffset + fieldPadding, valueY},
                                       invalid ? theme.MissingText : theme.TextPrimary,
                                       contentWidth - valueOffset - fieldPadding * 2.0f, value);
            return y + (stack ? lineHeight * 2.0f + 2.0f : lineHeight) + theme.SectionSpacing;
        }
    }

    void DetailsPanel::Draw(
        const EditorSession &session,
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect)
    {
        ResetHitAreas();
        if (IsEditing() && m_EditingObject != session.GetSelectedObjectID())
            CancelEdit();
        if (rect.Width <= 0.0f || rect.Height <= 0.0f)
            return;

        const float lineHeight = fontAtlas.GetLineHeight();
        const float left = rect.X + theme.PanelPadding;
        const float contentWidth = rect.Width - theme.PanelPadding * 2.0f;

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        SidePanelDrawing::DrawHeader(window, renderer, fontAtlas, theme, rect, "Details");

        float contentY = rect.Y + theme.PanelHeaderHeight + theme.SectionSpacing;
        const uint32_t selectedCount = session.GetSelectedObjectCount();
        if (selectedCount == 0)
        {
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, contentY},
                                       theme.TextMuted, contentWidth, "Select an object to inspect");
        } else if (selectedCount > 1)
        {
            const std::string countText =
                std::to_string(selectedCount) + " objects selected";
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, contentY},
                                       theme.TextPrimary, contentWidth, countText);
            contentY += lineHeight + theme.SectionSpacing;

            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "Transform");

            const Limen::LArray<Limen::SceneObjectID> &selected =
                session.GetSelectedObjectIDs();
            glm::mat4 first{1.0f};
            bool available = selected.getSize() > 0 &&
                             session.TryGetObjectTransform(selected[0], first);
            bool sameTransform = available;
            for (uint32_t i = 1; available && i < selected.getSize(); ++i)
            {
                glm::mat4 other{1.0f};
                if (!session.TryGetObjectTransform(selected[i], other))
                {
                    available = false;
                    break;
                }
                if (!(other == first))
                    sameTransform = false;
            }

            contentY = DrawPropertyRow(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "Location",
                !available
                    ? "Unavailable"
                    : (sameTransform
                           ? FormatVector(glm::vec3(first[3]))
                           : "Multiple values"));
        } else
        {
            Limen::LString label;
            const std::string labelText =
                session.TryGetSelectedObjectLabel(label) && !label.empty()
                    ? std::string(label.data(), label.size())
                    : "Unnamed object";
            SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, contentY},
                                       theme.TextPrimary, contentWidth, labelText);
            contentY += lineHeight + theme.SectionSpacing;

            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "Transform");

            Limen::SceneTransform transform;
            if (session.TryGetSelectedObjectTransform(transform))
            {
                m_PaintedObject = session.GetSelectedObjectID();
                const auto drawField = [&](const DetailsTransformField field, const std::string_view name,
                                           const glm::vec3 &value, const std::string_view unit = {})
                {
                    const bool focused = IsEditing() && m_Field == field;
                    const auto display = focused
                        ? m_Draft + (m_DraftSelected ? "" : "|") : FormatVector(value) + std::string(unit);
                    contentY = DrawPropertyRow(window, renderer, fontAtlas, theme, rect, contentY,
                                               name, display, &m_TransformFields[static_cast<std::size_t>(field)],
                                               focused, focused && m_InvalidDraft);
                };
                drawField(DetailsTransformField::Location, "Location", transform.Location);
                drawField(DetailsTransformField::Rotation, "Rotation",
                          glm::degrees(glm::eulerAngles(transform.Rotation)), " deg");
                drawField(DetailsTransformField::Scale, "Scale", transform.Scale);
                if (IsEditing())
                {
                    SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, contentY},
                        m_InvalidDraft ? theme.MissingText : theme.TextMuted, contentWidth,
                        m_InvalidDraft ? "Enter three numbers" : "Enter to apply   Esc to cancel");
                    contentY += lineHeight + theme.SectionSpacing;
                }
            }
            else
            {
                contentY = DrawPropertyRow(window, renderer, fontAtlas, theme, rect, contentY,
                                           "Location", "Unavailable");
            }

            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "Components");

            uint32_t componentCount = 0;
            const Limen::SceneObjectID selected =
                session.GetSelectedObjectID();
            session.ForEachObjectComponent(
                selected,
                [&](const Limen::SceneObjectComponent &component)
                {
                    ++componentCount;
                    if (contentY >= rect.Y + rect.Height)
                        return;

                    std::string modelDisplayName;
                    if (const auto *model =
                            std::get_if<Limen::ModelRendererComponent>(
                                &component.Data))
                    {
                        Limen::AssetRecord record;
                        if (session.GetAssetRegistry().TryFind(
                                model->ModelAssetID,
                                record))
                        {
                            modelDisplayName.assign(
                                record.LogicalPath.data(),
                                record.LogicalPath.size());
                            if (record.Status ==
                                Limen::AssetRecordStatus::Missing)
                            {
                                modelDisplayName += " [missing]";
                            }
                        }
                    }

                    const DetailsComponentDescription description =
                        DescribeSceneObjectComponent(
                            component,
                            modelDisplayName);
                    const float textIndent = 13.0f;
                    const float textWidth = contentWidth - textIndent;
                    if (textWidth > 0.0f)
                        renderer.DrawRoundedRect(window, {left, contentY + (lineHeight - 5.0f) * 0.5f},
                                                 {5.0f, 5.0f}, theme.Accent, 2.5f);
                    SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left + textIndent, contentY},
                                               theme.TextPrimary, textWidth, description.Title);
                    contentY += lineHeight;
                    SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left + textIndent, contentY},
                                               theme.TextMuted, textWidth, description.Summary);
                    contentY += lineHeight;
                    if (!description.Detail.empty())
                    {
                        SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left + textIndent, contentY},
                                                   theme.TextMuted, textWidth, description.Detail);
                        contentY += lineHeight;
                    }
                    contentY += theme.SectionSpacing;
                });

            if (componentCount == 0)
                SidePanelDrawing::DrawText(window, renderer, fontAtlas, {left, contentY},
                                           theme.TextMuted, contentWidth, "No components");
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }

    bool DetailsPanel::OnMousePressed(EditorSession &session, const float x, const float y)
    {
        const bool hasFields = m_PaintedObject.isValid() && m_PaintedObject == session.GetSelectedObjectID();
        auto field = m_TransformFields.size();
        if (hasFields)
        {
            for (std::size_t index = 0; index < m_TransformFields.size(); ++index)
            {
                if (m_TransformFields[index].Contains(x, y))
                {
                    field = index;
                    break;
                }
            }
        }

        if (IsEditing() && field == static_cast<std::size_t>(m_Field))
            return true;
        if (!CommitEdit(session))
            return true;
        if (field == m_TransformFields.size())
            return false;

        Limen::SceneTransform transform;
        if (!session.TryGetSelectedObjectTransform(transform))
            return false;
        m_Field = static_cast<DetailsTransformField>(field);
        switch (m_Field)
        {
            case DetailsTransformField::Location: m_Draft = EditValue(transform.Location); break;
            case DetailsTransformField::Rotation:
                m_Draft = EditValue(glm::degrees(glm::eulerAngles(transform.Rotation)));
                break;
            case DetailsTransformField::Scale: m_Draft = EditValue(transform.Scale); break;
        }
        m_EditingObject = m_PaintedObject;
        m_DraftSelected = true;
        return true;
    }

    bool DetailsPanel::OnKeyPressed(EditorSession &session, const Limen::KeyCode key)
    {
        if (!IsEditing())
            return false;
        using Limen::KeyCode;
        if (key == KeyCode::Escape)
            CancelEdit();
        else if (key == KeyCode::Enter || key == KeyCode::KeypadEnter)
            static_cast<void>(CommitEdit(session));
        else if (key == KeyCode::Backspace || key == KeyCode::Delete)
        {
            if (m_DraftSelected)
                m_Draft.clear();
            else if (!m_Draft.empty())
                m_Draft.pop_back();
            m_DraftSelected = false;
            m_InvalidDraft = false;
        }
        return true;
    }

    bool DetailsPanel::OnKeyTyped(const char32_t codepoint)
    {
        if (!IsEditing())
            return false;
        if ((codepoint >= U'0' && codepoint <= U'9') || codepoint == U'.' || codepoint == U',' ||
            codepoint == U' ' || codepoint == U'+' || codepoint == U'-' || codepoint == U'e' || codepoint == U'E')
        {
            if (m_DraftSelected)
                m_Draft.clear();
            if (m_Draft.size() < 96)
                m_Draft.push_back(static_cast<char>(codepoint));
            m_DraftSelected = false;
            m_InvalidDraft = false;
        }
        return true;
    }

    bool DetailsPanel::CommitEdit(EditorSession &session)
    {
        if (!IsEditing())
            return true;
        if (m_EditingObject != session.GetSelectedObjectID() || m_DraftSelected)
        {
            CancelEdit();
            return true;
        }

        Limen::SceneTransform transform;
        if (!session.TryGetSelectedObjectTransform(transform) ||
            !TryApplyDetailsTransformInput(m_Field, m_Draft, transform) ||
            !session.SetSelectedObjectTransform(transform))
        {
            m_InvalidDraft = true;
            return false;
        }

        CancelEdit();
        return true;
    }

    void DetailsPanel::ResetHitAreas() noexcept
    {
        m_TransformFields = {};
        m_PaintedObject = {};
    }

    void DetailsPanel::CancelEdit()
    {
        m_EditingObject = {};
        m_Draft.clear();
        m_DraftSelected = false;
        m_InvalidDraft = false;
    }
}
