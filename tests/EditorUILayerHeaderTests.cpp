// 此头文件必须能独立包含；不能依赖 EditorApp.cpp 恰好先包含 EditorSession.h。
#include "UI/EditorUILayer.h"

#include <type_traits>

static_assert(std::is_constructible_v<
    LimenEditor::EditorUILayer,
    Limen::Ref<LimenEditor::EditorSession>
>);

int main()
{
    return 0;
}
