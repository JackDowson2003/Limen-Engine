#include <chrono>
#include <iostream>
#include <string_view>

#include "UI/EditorStatusMessage.h"

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
    const Clock::time_point start{};

    bool success = true;
    LimenEditor::EditorStatusMessage message;

    success = Check(
                  "empty message is hidden",
                  message.GetText(start).empty()) &&
              success;

    message.Show(
        "Placed asset.",
        LimenEditor::EditorStatusKind::Info,
        start);
    success = Check(
                  "active message is visible",
                  message.GetText(
                      start + std::chrono::milliseconds(100)) ==
                  std::string_view{"Placed asset."}) &&
              success;
    success = Check(
                  "info kind is retained",
                  message.GetKind(
                      start + std::chrono::milliseconds(100)) ==
                  LimenEditor::EditorStatusKind::Info) &&
              success;

    message.Show(
        "Load failed.",
        LimenEditor::EditorStatusKind::Error,
        start + std::chrono::milliseconds(200),
        std::chrono::milliseconds(500));
    success = Check(
                  "error kind is retained",
                  message.GetKind(
                      start + std::chrono::milliseconds(300)) ==
                  LimenEditor::EditorStatusKind::Error) &&
              success;
    success = Check(
                  "message expires",
                  message.GetText(
                      start + std::chrono::milliseconds(700)).empty()) &&
              success;

    message.Show(
        "Temporary",
        LimenEditor::EditorStatusKind::Info,
        start);
    message.Clear();
    success = Check(
                  "clear hides the message",
                  message.GetText(start).empty()) &&
              success;

    if (success)
        std::cout << "Editor status message: PASS\n";
    return success ? 0 : 1;
}
