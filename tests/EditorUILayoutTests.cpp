#include <cmath>
#include <iostream>

#include "UI/EditorUILayout.h"

namespace
{
    bool CheckNear(const char *name, const float actual, const float expected)
    {
        constexpr float tolerance = 0.001f;
        if (std::abs(actual - expected) <= tolerance)
            return true;

        std::cerr << name << ": expected " << expected << ", got " << actual << '\n';
        return false;
    }

    bool Check(const char *name, const bool condition)
    {
        if (condition)
            return true;

        std::cerr << name << ": failed\n";
        return false;
    }
}

int main()
{
    int failures = 0;

    // 视口与抽屉共用左区，Outliner 和属性面板共用右区。
    const auto normal = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.24f, true, false);
    if (!CheckNear("normal toolbar keeps commands on one row", normal.Top.Height, 62.0f))
        ++failures;
    if (!CheckNear("normal outliner width", normal.Outliner.Width, 384.0f))
        ++failures;
    if (!CheckNear("normal render viewport width", normal.Center.Width, 1216.0f))
        ++failures;
    if (!CheckNear("details shares the outliner width", normal.Inspector.Width, normal.Outliner.Width) ||
        !CheckNear("details shares the outliner column", normal.Inspector.X, normal.Outliner.X) ||
        !CheckNear("right column starts after viewport", normal.Outliner.X, normal.Center.X + normal.Center.Width))
        ++failures;
    if (!CheckNear("viewport toolbar ends at the rendered scene",
                   normal.ViewportToolbar.Y + normal.ViewportToolbar.Height, normal.Center.Y))
        ++failures;
    if (!CheckNear("content splitter touches rendered viewport",
                   normal.Center.Y + normal.Center.Height, normal.ContentSplitter.Y))
        ++failures;
    if (!CheckNear("content spans the viewport", normal.Content.Width, normal.Center.Width) ||
        !CheckNear("outliner borders the right-side splitter",
                   normal.Outliner.Y + normal.Outliner.Height, normal.DetailsSplitter.Y) ||
        !CheckNear("details starts immediately after the right-side splitter",
                   normal.DetailsSplitter.Y + normal.DetailsSplitter.Height, normal.Inspector.Y) ||
        !CheckNear("details reaches the status bar",
                   normal.Inspector.Y + normal.Inspector.Height, normal.Status.Y))
        ++failures;
    if (!CheckNear("outliner uses default right-side split ratio",
                   normal.Outliner.Height, LimenEditor::kDefaultEditorRightSplitRatio *
                   (normal.Status.Y - normal.Top.Height - normal.DetailsSplitter.Height)))
        ++failures;
    if (!CheckNear("content fills its fraction of body after divider",
                   normal.Content.Height, (normal.Status.Y - normal.Top.Height - normal.ContentSplitter.Height) * 0.24f) ||
        !CheckNear("drawer ends at the status bar", normal.Content.Y + normal.Content.Height, normal.Status.Y) ||
        !CheckNear("status bar ends at bottom edge", normal.Status.Y + normal.Status.Height, 900.0f))
        ++failures;

    const auto hidden = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.24f, false, false);
    if (!Check("closing the drawer clears both geometry and hit area",
               hidden.Content.Height == 0.0f && hidden.ContentSplitter.Height == 0.0f) ||
        !CheckNear("the scene takes space returned by the drawer",
                   hidden.Center.Height, normal.Center.Height + normal.Content.Height + normal.ContentSplitter.Height) ||
        !CheckNear("closing drawer preserves right-side splitter position",
                   hidden.DetailsSplitter.Y, normal.DetailsSplitter.Y))
        ++failures;

    const auto expanded = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.24f, true, true);
    if (!Check("expanded viewport clears other panel hit areas",
               expanded.Outliner.Width == 0.0f && expanded.Inspector.Width == 0.0f &&
               expanded.DetailsSplitter.Height == 0.0f &&
               expanded.Content.Height == 0.0f && expanded.ContentSplitter.Height == 0.0f) ||
        !CheckNear("expanded scene fills window width", expanded.Center.Width, 1600.0f) ||
        !CheckNear("expanded mode still provides toolbar and status",
                   expanded.Center.Y + expanded.Center.Height, expanded.Status.Y) ||
        !Check("expanded toolbar stays visible", expanded.ViewportToolbar.Height > 0.0f))
        ++failures;

    // 更大的窗口优先把额外宽度交给视口。
    const auto enlarged = LimenEditor::CalculateEditorUILayout(3200.0f, 1800.0f, 0.24f, true, false);
    if (!CheckNear("enlarged right column respects its maximum width", enlarged.Outliner.Width, 405.0f) ||
        !CheckNear("enlarged viewport takes remaining width", enlarged.Center.Width, 2795.0f))
        ++failures;
    if (!CheckNear("enlarged bottom edge",
                   enlarged.Status.Y + enlarged.Status.Height, 1800.0f))
        ++failures;

    // 拖到两端也会给资源和实际场景留出可用空间。
    const auto smallDrawer = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, -1.0f, true, false);
    const auto largeDrawer = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 4.0f, true, false);
    if (!CheckNear("normal window clamps drawer to its minimum height", smallDrawer.Content.Height, 125.0f) ||
        !Check("large drawer preserves enough scene height", largeDrawer.Center.Height >= 150.0f))
        ++failures;

    const auto tallOutliner = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.24f, true, false, 0.72f);
    const auto smallestOutliner = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.24f, true, false, -1.0f);
    const auto smallestDetails = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.24f, true, false, 4.0f);
    if (!Check("changing split ratio moves right-side splitter", tallOutliner.DetailsSplitter.Y > normal.DetailsSplitter.Y) ||
        !CheckNear("resizing right panels preserves viewport height", tallOutliner.Center.Height, normal.Center.Height) ||
        !CheckNear("resizing right panels preserves drawer height", tallOutliner.Content.Height, normal.Content.Height) ||
        !CheckNear("outliner keeps a usable height", smallestOutliner.Outliner.Height, 78.0f) ||
        !CheckNear("details keeps a usable height", smallestDetails.Inspector.Height, 78.0f))
        ++failures;

    const auto shortWindow = LimenEditor::CalculateEditorUILayout(800.0f, 130.0f, 0.5f, true, false);
    if (!Check("short window hides drawer and viewport tools before sharing the canvas",
               shortWindow.Content.Height == 0.0f && shortWindow.ContentSplitter.Height == 0.0f &&
               shortWindow.DetailsSplitter.Height == 0.0f && shortWindow.Outliner.Width == 0.0f &&
               shortWindow.ViewportToolbar.Height == 0.0f && shortWindow.Center.Height > 0.0f) ||
        !CheckNear("short window keeps the viewport within its bounds",
                   shortWindow.Center.Y + shortWindow.Center.Height, shortWindow.Status.Y))
        ++failures;

    const auto shallowDrawer = LimenEditor::CalculateEditorUILayout(1600.0f, 390.0f, 0.24f, true, false);
    if (!Check("drawer is omitted when its asset cards would be unusable",
               shallowDrawer.Content.Height == 0.0f && shallowDrawer.Center.Height > 150.0f))
        ++failures;

    const auto narrowWindow = LimenEditor::CalculateEditorUILayout(600.0f, 900.0f, 0.24f, true, false);
    const auto tinyWindow = LimenEditor::CalculateEditorUILayout(540.0f, 900.0f, 0.24f, true, false);
    if (!CheckNear("narrow toolbar reserves a second command row", narrowWindow.Top.Height, 92.0f) ||
        !CheckNear("narrow window protects scene width", narrowWindow.Center.Width, 400.0f) ||
        !CheckNear("very narrow window lets right panels collapse", tinyWindow.Center.Width, 540.0f) ||
        !Check("right-hand splitter hides alongside right panels", tinyWindow.DetailsSplitter.Height == 0.0f))
        ++failures;

    const auto minimized = LimenEditor::CalculateEditorUILayout(800.0f, 0.0f, 0.24f, true, false);
    if (!Check("minimized layout has no active panel geometry",
               !minimized.ContentSplitter.Contains(100.0f, 100.0f) && minimized.Status.Height == 0.0f))
        ++failures;
    if (!Check("splitter includes top-left edge",
               normal.ContentSplitter.Contains(normal.ContentSplitter.X, normal.ContentSplitter.Y)))
        ++failures;
    if (!Check("splitter excludes bottom edge",
               !normal.ContentSplitter.Contains(normal.ContentSplitter.X,
                                                normal.ContentSplitter.Y + normal.ContentSplitter.Height)))
        ++failures;

    // 视口像素尺寸：逻辑矩形按帧缓冲/逻辑比例换算，且至少 1×1。
    const auto scaled = LimenEditor::CalculateViewportPixelSize(
        1600.0f, 900.0f, normal.Center, 3200, 1800);
    if (!Check("viewport width maps with 2x framebuffer scale", scaled.Width == 2432))
        ++failures;
    if (!Check("viewport height maps with 2x framebuffer scale", scaled.Height == 1152))
        ++failures;

    const auto identity = LimenEditor::CalculateViewportPixelSize(
        1600.0f, 900.0f, normal.Center, 1600, 900);
    if (!Check("viewport identity scale keeps logical size",
               identity.Width == 1216 && identity.Height == 576))
        ++failures;

    const auto tiny = LimenEditor::CalculateViewportPixelSize(
        1600.0f, 900.0f, {0.0f, 128.0f, 0.2f, 0.2f}, 1600, 900);
    if (!Check("tiny logical rect clamps to at least 1x1",
               tiny.Width == 1 && tiny.Height == 1))
        ++failures;

    const auto invalid = LimenEditor::CalculateViewportPixelSize(
        0.0f, 900.0f, normal.Center, 1600, 900);
    if (!Check("zero logical window returns 1x1",
               invalid.Width == 1 && invalid.Height == 1))
        ++failures;

    // 列表行命中：首行、行边界、末行外与非法行高。
    if (!Check("row above list returns -1",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 99.0f, 3) == -1))
        ++failures;
    if (!Check("first row starts at firstRowY",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 100.0f, 3) == 0))
        ++failures;
    if (!Check("row inside first slot",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 110.0f, 3) == 0))
        ++failures;
    if (!Check("boundary starts next row",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 120.0f, 3) == 1))
        ++failures;
    if (!Check("last row is reachable",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 150.0f, 3) == 2))
        ++failures;
    if (!Check("row below list returns -1",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 160.0f, 3) == -1))
        ++failures;
    if (!Check("zero row height returns -1",
               LimenEditor::FindListRowAt(100.0f, 0.0f, 110.0f, 3) == -1))
        ++failures;
    if (!Check("zero row count returns -1",
               LimenEditor::FindListRowAt(100.0f, 20.0f, 110.0f, 0) == -1))
        ++failures;

    // Shift 范围选择：使用 Outliner 的行顺序，不依赖选择集合内部顺序。
    const Limen::SceneObjectID idA{1, 1};
    const Limen::SceneObjectID idB{2, 2};
    const Limen::SceneObjectID idC{3, 3};
    const Limen::SceneObjectID idMissing{99, 99};

    Limen::LArray<Limen::SceneObjectID> rows;
    rows.push_back(idA);
    rows.push_back(idB);
    rows.push_back(idC);

    const auto forwardRange =
        LimenEditor::BuildSelectionRange(rows, idA, idC);
    if (!Check("range selects anchor through target",
               forwardRange.getSize() == 3 &&
               forwardRange[0] == idA &&
               forwardRange[1] == idB &&
               forwardRange[2] == idC))
        ++failures;

    const auto reverseRange =
        LimenEditor::BuildSelectionRange(rows, idC, idA);
    if (!Check("reverse range has same normalized order",
               reverseRange.getSize() == 3 &&
               reverseRange[0] == idA &&
               reverseRange[1] == idB &&
               reverseRange[2] == idC))
        ++failures;

    const auto singleRange =
        LimenEditor::BuildSelectionRange(rows, idB, idB);
    if (!Check("same anchor and target selects one row",
               singleRange.getSize() == 1 && singleRange[0] == idB))
        ++failures;

    const auto missingAnchorRange =
        LimenEditor::BuildSelectionRange(rows, idMissing, idB);
    if (!Check("missing anchor falls back to target only",
               missingAnchorRange.getSize() == 1 &&
               missingAnchorRange[0] == idB))
        ++failures;

    const auto missingTargetRange =
        LimenEditor::BuildSelectionRange(rows, idB, idMissing);
    if (!Check("missing target yields empty range",
               missingTargetRange.getSize() == 0))
        ++failures;

    return failures == 0 ? 0 : 1;
}
