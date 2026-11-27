#include "UI/EditorSceneDialogs.h"

namespace LimenEditor
{
    EditorSceneDialogResult ChooseSceneToOpen(
        const std::filesystem::path &,
        std::filesystem::path &)
    {
        return EditorSceneDialogResult::Unavailable;
    }

    EditorSceneDialogResult ChooseSceneToSave(
        const std::filesystem::path &,
        const std::filesystem::path &,
        std::filesystem::path &)
    {
        return EditorSceneDialogResult::Unavailable;
    }

    EditorUnsavedSceneAction ConfirmUnsavedScene()
    {
        return EditorUnsavedSceneAction::Cancel;
    }
}
