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

    // 布局使用逻辑窗口尺寸；顶部和分隔条占高后，剩余高度才分给两块面板。
    const auto normal = LimenEditor::CalculateEditorUILayout(1600.0f, 900.0f, 0.5f);
    if (!CheckNear("normal top height", normal.Top.Height, 128.0f))
        ++failures;
    if (!CheckNear("normal left width", normal.Left.Width, 280.0f))
        ++failures;
    if (!CheckNear("normal center width", normal.Center.Width, 1040.0f))
        ++failures;
    if (!CheckNear("normal right width", normal.RightTop.Width, 280.0f))
        ++failures;
    if (!CheckNear("normal splitter y", normal.RightSplitter.Y, 512.0f))
        ++failures;
    if (!CheckNear("normal splitter height", normal.RightSplitter.Height, 4.0f))
        ++failures;
    if (!CheckNear("normal bottom y", normal.RightBottom.Y, 516.0f))
        ++failures;
    if (!CheckNear("normal right body height",
                   normal.RightTop.Height + normal.RightSplitter.Height + normal.RightBottom.Height,
                   772.0f))
        ++failures;

    // 非全屏最大化只改变传入尺寸；三栏仍保持相同的宽度比例。
    const auto enlarged = LimenEditor::CalculateEditorUILayout(3200.0f, 1800.0f, 0.5f);
    if (!CheckNear("enlarged left ratio", enlarged.Left.Width / 3200.0f, 0.175f))
        ++failures;
    if (!CheckNear("enlarged center ratio", enlarged.Center.Width / 3200.0f, 0.65f))
        ++failures;
    if (!CheckNear("enlarged right ratio", enlarged.RightTop.Width / 3200.0f, 0.175f))
        ++failures;
    if (!CheckNear("enlarged bottom edge",
                   enlarged.RightBottom.Y + enlarged.RightBottom.Height, 1800.0f))
        ++failures;

    // 极矮窗口只容得下顶部与分隔条时，两块面板都不能得到负高度。
    const auto shortWindow = LimenEditor::CalculateEditorUILayout(800.0f, 130.0f, 0.5f);
    if (!CheckNear("short window splitter height", shortWindow.RightSplitter.Height, 2.0f))
        ++failures;
    if (!CheckNear("short window top panel height", shortWindow.RightTop.Height, 0.0f))
        ++failures;
    if (!CheckNear("short window bottom panel height", shortWindow.RightBottom.Height, 0.0f))
        ++failures;
    if (!CheckNear("short window bottom edge", shortWindow.RightBottom.Y, 130.0f))
        ++failures;

    const auto minimized = LimenEditor::CalculateEditorUILayout(800.0f, 0.0f, 0.5f);
    if (!Check("minimized layout has no splitter",
               !minimized.RightSplitter.Contains(100.0f, 100.0f)))
        ++failures;
    if (!Check("splitter includes top-left edge",
               normal.RightSplitter.Contains(normal.RightSplitter.X, normal.RightSplitter.Y)))
        ++failures;
    if (!Check("splitter excludes bottom edge",
               !normal.RightSplitter.Contains(normal.RightSplitter.X,
                                              normal.RightSplitter.Y + normal.RightSplitter.Height)))
        ++failures;

    return failures == 0 ? 0 : 1;
}
