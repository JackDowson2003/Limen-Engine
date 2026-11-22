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

    LimenEditor::ContentBrowserCommandGuard placeGuard;
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

    if (success)
        std::cout << "Content Browser click sequence: PASS\n";
    return success ? 0 : 1;
}
