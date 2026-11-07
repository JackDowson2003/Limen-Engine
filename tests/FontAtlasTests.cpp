#include <filesystem>
#include <iostream>
#include <string_view>

#include "Limen/Core/Log.h"
#include "Limen/Renderer/FontAtlas.h"

namespace
{
    int g_Failures = 0;

    void Check(const char *name, const bool passed)
    {
        if (passed)
            return;

        ++g_Failures;
        std::cerr << "FAIL: " << name << '\n';
    }
}

int main()
{
    namespace fs = std::filesystem;

    // 生产代码的失败分支会写日志，先初始化日志系统。
    Limen::Log::Init();

    const fs::path fontPath = LIMEN_TEST_FONT_PATH;

    // 缺失路径必须失败。
    Limen::FontAtlas missingAtlas;
    Check("loading a missing font file fails",
          !missingAtlas.LoadFontFile(fs::path{"__no_such_font__.otf"}));

    Limen::FontAtlas atlas(1024, 32.0f);
    Check("fresh atlas is not built", !atlas.IsBuilt());
    Check("font height is retained before build",
          atlas.GetFontHeight() == 32.0f);
    Check("line height is zero before metrics are computed",
          atlas.GetLineHeight() == 0.0f);

    // 未加载字体字节时，Build 必须被卫语句拦下。
    atlas.AddText("A");
    Check("build without font bytes fails", !atlas.Build());

    Check("loading the built-in font succeeds",
          atlas.LoadFontFile(fontPath));

    // 未登记任何码点时 Build 同样必须失败。
    Limen::FontAtlas emptyGlyphAtlas;
    Check("empty glyph set is rejected",
          emptyGlyphAtlas.LoadFontFile(fontPath) &&
          !emptyGlyphAtlas.Build());

    atlas.AddText("Ab 场景 view");
    atlas.AddText("A");   // 重复登记不改变结果
    Check("ASCII glyph is registered",
          atlas.FindGlyph(U'A') != nullptr);
    Check("CJK glyph is registered",
          atlas.FindGlyph(U'场') != nullptr);
    Check("unregistered glyph is absent",
          atlas.FindGlyph(U'Z') == nullptr);

    // 非法 UTF-8 字节应落到 U+FFFD 占位符，而不是崩溃。
    atlas.AddText(std::string_view{"\xFF\xFE", 2});
    Check("invalid UTF-8 falls back to replacement glyph",
          atlas.FindGlyph(U'\uFFFD') != nullptr);

    if (g_Failures == 0)
        std::cout << "Font atlas loading, glyph registry, and guards: PASS\n";
    return g_Failures == 0 ? 0 : 1;
}
