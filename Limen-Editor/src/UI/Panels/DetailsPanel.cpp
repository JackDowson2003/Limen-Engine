//
// Created by chenlong on 2026/11/21.
//
#include "UI/Panels/DetailsPanel.h"

#include <algorithm>
#include <iomanip>
#include <sstream>
#include <variant>

#include "Limen/Application/Window.h"
#include "Limen/Renderer/FontAtlas.h"
#include "Limen/Scene/SceneComponent.h"
#include "Project/EditorSession.h"
#include "Render/EditorUIRenderer.h"
#include "UI/Panels/DetailsComponentText.h"
#include "UI/EditorTheme.h"

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

        std::string FormatLocation(const glm::mat4 &transform)
        {
            return FormatFloat(transform[3][0]) + ", " +
                   FormatFloat(transform[3][1]) + ", " +
                   FormatFloat(transform[3][2]);
        }

        float DrawSectionHeader(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect,
            const float y,
            const char *label)
        {
            const float lineHeight = fontAtlas.GetLineHeight();
            renderer.DrawSolidRect(
                window,
                {rect.X + 8.0f, y},
                {rect.Width - 16.0f, lineHeight},
                theme.SectionBackground);
            renderer.DrawSolidRect(
                window,
                {rect.X + 8.0f, y},
                {2.0f, lineHeight},
                theme.Accent);
            renderer.DrawText(
                window,
                {rect.X + 16.0f, y},
                theme.TextSecondary,
                fontAtlas,
                label);
            return y + lineHeight + 4.0f;
        }

        float DrawPropertyRow(
            const Limen::Window &window,
            const EditorUIRenderer &renderer,
            const Limen::FontAtlas &fontAtlas,
            const EditorTheme &theme,
            const UIRect &rect,
            const float y,
            const char *name,
            const std::string_view value)
        {
            const float lineHeight = fontAtlas.GetLineHeight();
            renderer.DrawText(
                window,
                {rect.X + 16.0f, y},
                theme.TextMuted,
                fontAtlas,
                name);
            renderer.DrawText(
                window,
                {rect.X + 82.0f, y},
                theme.TextSecondary,
                fontAtlas,
                value);
            return y + lineHeight + 2.0f;
        }
    }

    void DetailsPanel::Draw(
        const EditorSession &session,
        const Limen::Window &window,
        const EditorUIRenderer &renderer,
        const Limen::FontAtlas &fontAtlas,
        const EditorTheme &theme,
        const UIRect &rect) const
    {
        const float lineHeight = fontAtlas.GetLineHeight();
        const float headerHeight = theme.PanelHeaderHeight;
        const float headerY =
            rect.Y + std::max(0.0f, (headerHeight - lineHeight) * 0.5f);

        renderer.SetClipRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, rect.Height});

        renderer.DrawSolidRect(
            window,
            {rect.X, rect.Y},
            {rect.Width, headerHeight},
            theme.PanelHeaderBackground);
        renderer.DrawSolidRect(
            window,
            {rect.X, rect.Y + headerHeight - 1.0f},
            {rect.Width, 1.0f},
            theme.PanelBorder);
        renderer.DrawSolidRect(
            window,
            {rect.X + 8.0f, headerY},
            {2.0f, lineHeight},
            theme.Accent);
        renderer.DrawText(
            window,
            {rect.X + 16.0f, headerY},
            theme.TextPrimary,
            fontAtlas,
            "DETAILS");

        float contentY = rect.Y + headerHeight + theme.SectionSpacing;
        const uint32_t selectedCount = session.GetSelectedObjectCount();
        if (selectedCount == 0)
        {
            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "SELECTION");
            renderer.DrawText(
                window,
                {rect.X + 16.0f, contentY},
                theme.TextMuted,
                fontAtlas,
                "No actor selected");
        } else if (selectedCount > 1)
        {
            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "SELECTION");
            const std::string countText =
                std::to_string(selectedCount) + " actors selected";
            renderer.DrawText(
                window,
                {rect.X + 16.0f, contentY},
                theme.TextSecondary,
                fontAtlas,
                countText);
            contentY += lineHeight + theme.SectionSpacing;

            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "TRANSFORM");

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
                           ? FormatLocation(first)
                           : "Multiple values"));
        } else
        {
            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "SELECTION");

            Limen::LString label;
            const std::string labelText =
                session.TryGetSelectedObjectLabel(label) && !label.empty()
                    ? std::string(label.data(), label.size())
                    : "Unnamed actor";
            contentY = DrawPropertyRow(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "Label",
                labelText);

            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "TRANSFORM");

            glm::mat4 transform{1.0f};
            contentY = DrawPropertyRow(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "Location",
                session.TryGetSelectedObjectTransform(transform)
                    ? FormatLocation(transform)
                    : "Unavailable");

            contentY = DrawSectionHeader(
                window,
                renderer,
                fontAtlas,
                theme,
                rect,
                contentY,
                "COMPONENTS");

            uint32_t componentCount = 0;
            const Limen::SceneObjectID selected =
                session.GetSelectedObjectID();
            session.ForEachObjectComponent(
                selected,
                [&](const Limen::SceneObjectComponent &component)
                {
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
                    const uint32_t textRows =
                        description.Detail.empty() ? 2U : 3U;
                    const float cardHeight =
                        lineHeight * static_cast<float>(textRows) + 8.0f;

                    renderer.DrawSolidRect(
                        window,
                        {rect.X + 8.0f, contentY},
                        {rect.Width - 16.0f, cardHeight},
                        theme.SectionBackground);
                    renderer.DrawSolidRect(
                        window,
                        {rect.X + 8.0f, contentY},
                        {2.0f, cardHeight},
                        theme.Accent);
                    renderer.DrawText(
                        window,
                        {rect.X + 18.0f, contentY + 3.0f},
                        theme.TextSecondary,
                        fontAtlas,
                        description.Title);
                    renderer.DrawText(
                        window,
                        {rect.X + 26.0f,
                         contentY + lineHeight + 3.0f},
                        theme.TextMuted,
                        fontAtlas,
                        description.Summary);
                    if (!description.Detail.empty())
                    {
                        renderer.DrawText(
                            window,
                            {rect.X + 26.0f,
                             contentY + lineHeight * 2.0f + 3.0f},
                            theme.TextMuted,
                            fontAtlas,
                            description.Detail);
                    }

                    contentY += cardHeight + 4.0f;
                    ++componentCount;
                });

            if (componentCount == 0)
            {
                renderer.DrawText(
                    window,
                    {rect.X + 16.0f, contentY},
                    theme.TextMuted,
                    fontAtlas,
                    "No authoring components");
            }
        }

        renderer.SetClipRect(window, {0.0f, 0.0f}, {0.0f, 0.0f});
    }
}
