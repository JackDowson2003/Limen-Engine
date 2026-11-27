#pragma once

#include <filesystem>

namespace LimenEditor
{
    enum class EditorSceneDialogResult
    {
        Accepted,
        Cancelled,
        Unavailable
    };

    // 由 Editor 主线程在窗口事件轮询结束后调用。只有 Accepted 会改写 outPath。
    [[nodiscard]] EditorSceneDialogResult ChooseSceneToOpen(
        const std::filesystem::path &initialDirectory,
        std::filesystem::path &outPath);

    [[nodiscard]] EditorSceneDialogResult ChooseSceneToSave(
        const std::filesystem::path &initialDirectory,
        const std::filesystem::path &suggestedFileName,
        std::filesystem::path &outPath);

    enum class EditorUnsavedSceneAction
    {
        Save,
        Discard,
        Cancel
    };

    [[nodiscard]] EditorUnsavedSceneAction ConfirmUnsavedScene();
}
