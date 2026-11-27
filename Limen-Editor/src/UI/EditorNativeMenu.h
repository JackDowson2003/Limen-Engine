#pragma once

#include <functional>

namespace LimenEditor
{
    enum class SceneToolbarAction;

    // 在应用主线程安装系统菜单。调用方须在回调目标销毁前撤销菜单。
    void InstallEditorNativeMenu(
        bool hasProject,
        std::function<void(SceneToolbarAction)> sceneAction,
        std::function<void()> deleteSelection);
    void UninstallEditorNativeMenu();
}
