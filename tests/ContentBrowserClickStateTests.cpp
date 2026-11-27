#include <chrono>
#include <iostream>

#include "UI/Panels/ContentBrowserPanel.h"

namespace
{
    bool Check(const char *name, const bool condition)
    {
        if (condition)
            return true;

        std::cerr << "FAIL: " << name << '\n';
        return false;
    }
}

int main()
{
    using Clock = std::chrono::steady_clock;
    using Result = LimenEditor::ContentBrowserClickState::Result;

    const Limen::AssetID first{
        0x0011223344556677ULL,
        0x8899aabbccddeeffULL
    };
    const Limen::AssetID second{
        0x1111222233334444ULL,
        0x5555666677778888ULL
    };
    const Clock::time_point start{};

    bool success = true;
    LimenEditor::ContentBrowserClickState clickState;

    success = Check(
                  "first press selects",
                  clickState.RegisterPress(first, start) ==
                  Result::SelectionChanged) &&
              success;
    success = Check(
                  "second press within interval activates once",
                  clickState.RegisterPress(
                      first,
                      start + std::chrono::milliseconds(200)) ==
                  Result::ActivateAsset) &&
              success;
    success = Check(
                  "press after activation starts a new sequence",
                  clickState.RegisterPress(
                      first,
                      start + std::chrono::milliseconds(250)) ==
                  Result::SelectionChanged) &&
              success;
    success = Check(
                  "next press activates the new sequence",
                  clickState.RegisterPress(
                      first,
                      start + std::chrono::milliseconds(300)) ==
                  Result::ActivateAsset) &&
              success;

    LimenEditor::ContentBrowserClickState changedAsset;
    static_cast<void>(changedAsset.RegisterPress(first, start));
    success = Check(
                  "a different asset starts a new sequence",
                  changedAsset.RegisterPress(
                      second,
                      start + std::chrono::milliseconds(100)) ==
                  Result::SelectionChanged) &&
              success;

    LimenEditor::ContentBrowserClickState expiredClick;
    static_cast<void>(expiredClick.RegisterPress(first, start));
    success = Check(
                  "click after the interval selects",
                  expiredClick.RegisterPress(
                      first,
                      start + std::chrono::milliseconds(351)) ==
                  Result::SelectionChanged) &&
              success;

    expiredClick.Reset();
    success = Check(
                  "reset starts a new sequence",
                  expiredClick.RegisterPress(
                      first,
                      start + std::chrono::milliseconds(352)) ==
                  Result::SelectionChanged) &&
              success;

    LimenEditor::EditorCommandGuard placeGuard;
    success = Check(
                  "place command triggers once",
                  placeGuard.TryAcquire(start)) &&
              success;
    success = Check(
                  "repeated place press is ignored",
                  !placeGuard.TryAcquire(
                      start + std::chrono::milliseconds(200))) &&
              success;
    success = Check(
                  "place command recovers after interval",
                  placeGuard.TryAcquire(
                      start + std::chrono::milliseconds(351))) &&
              success;
    placeGuard.Reset();
    success = Check(
                  "place guard reset allows immediate trigger",
                  placeGuard.TryAcquire(
                      start + std::chrono::milliseconds(352))) &&
              success;

    LimenEditor::ContentBrowserDragState dragState;
    dragState.Begin(first, 10.0f, 20.0f);
    dragState.Update(12.0f, 22.0f);
    success = Check(
                  "small pointer movement does not begin dragging",
                  !dragState.IsDragging()) &&
              success;
    dragState.Update(20.0f, 20.0f);
    success = Check(
                  "drag begins after threshold",
                  dragState.IsDragging()) &&
              success;
    success = Check(
                  "drag release reports an active drag",
                  dragState.End()) &&
              success;
    success = Check(
                  "drag state is cleared after release",
                  !dragState.IsDragging()) &&
              success;
    success = Check(
                  "click release without movement is not a drag",
                  !dragState.End()) &&
              success;

    const auto grid = LimenEditor::ContentBrowserGrid::Calculate({10.0f, 40.0f, 312.0f, 118.0f}, 5U);
    success = Check("tiles use tray width and wrap onto a third row",
                    grid.Columns == 2U && grid.TileAt(2U, 0.0f).Y == 126.0f) && success;
    success = Check("first two tiles are clickable",
                    grid.HitTest(20.0f, 50.0f, 0.0f) == 0U &&
                    grid.HitTest(165.0f, 50.0f, 0.0f) == 1U) && success;
    success = Check("gaps and clipped portions are not interactive",
                    !grid.HitTest(157.0f, 50.0f, 0.0f) &&
                    !grid.HitTest(20.0f, 39.0f, 86.0f)) && success;
    success = Check("scrolling matches drawn tiles and reaches the final asset",
                    grid.HitTest(20.0f, 50.0f, 86.0f) == 2U &&
                    grid.ScrollLimit() == 132.0f &&
                    grid.HitTest(20.0f, 100.0f, grid.ScrollLimit()) == 4U &&
                    !grid.HitTest(165.0f, 100.0f, grid.ScrollLimit())) && success;
    const auto narrow = LimenEditor::ContentBrowserGrid::Calculate({0.0f, 0.0f, 90.0f, 120.0f}, 2U);
    success = Check("narrow tray fits one tile and empty tray has no hit targets",
                    narrow.Columns == 1U && narrow.TileWidth == 90.0f &&
                    !LimenEditor::ContentBrowserGrid{}.HitTest(0.0f, 0.0f, 0.0f)) && success;

    using LimenEditor::ContentAssetKind;
    using LimenEditor::DescribeContentAssetPath;
    const auto mesh = DescribeContentAssetPath("models.v2/Keep.Damaged.OBJ");
    const auto scene = DescribeContentAssetPath("scenes/ForestOutpost.lscene");
    success = Check("mesh and scene cards distinguish kind and use the filename stem",
                    mesh.Kind == ContentAssetKind::Mesh && mesh.Name == "Keep.Damaged" &&
                    scene.Kind == ContentAssetKind::Scene && scene.Name == "ForestOutpost") && success;
    success = Check("filename extraction preserves the UTF-8 asset name",
                    DescribeContentAssetPath("模型/城堡.obj").Name == "城堡") && success;
    success = Check("textures and material sources have their own kinds",
                    DescribeContentAssetPath("textures/leaves.PNG").Kind == ContentAssetKind::Image &&
                    DescribeContentAssetPath("models/keep.mtl").Kind == ContentAssetKind::Material) && success;
    const auto unknown = DescribeContentAssetPath("models.v2/Keep.fbx");
    success = Check("unrecognized and extensionless files retain their visible name",
                    unknown.Kind == ContentAssetKind::File && unknown.Name == "Keep.fbx" &&
                    DescribeContentAssetPath("models.v2/README").Name == "README" &&
                    DescribeContentAssetPath(".lscene").Kind == ContentAssetKind::File) && success;

    using LimenEditor::RelativeContentPath;
    success = Check("asset root exposes paths for folder tiles",
                    RelativeContentPath("Models/Props/keep.obj", "") == "Models/Props/keep.obj") && success;
    success = Check("nested paths and local assets appear in the right folder",
                    RelativeContentPath("Models/Props/keep.obj", "Models") == "Props/keep.obj" &&
                    RelativeContentPath("Models/Props/keep.obj", "Models/Props") == "keep.obj") && success;
    success = Check("sibling folders and partial matches are excluded",
                    RelativeContentPath("Models2/keep.obj", "Models").empty() &&
                    RelativeContentPath("Scenes/ForestOutpost.lscene", "Models").empty() &&
                    RelativeContentPath("Models", "Models").empty()) && success;

    if (success)
        std::cout << "Content Browser interactions: PASS\n";
    return success ? 0 : 1;
}
