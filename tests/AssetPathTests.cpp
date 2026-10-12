#include "Limen/Asset/AssetPath.h"

#include <filesystem>
#include <iostream>

namespace
{
    bool CheckPath(
        const char *caseName,
        const std::filesystem::path &actual,
        const std::filesystem::path &expected
    )
    {
        if (actual == expected)
            return true;

        std::cerr << caseName << ": expected '" << expected.generic_string()
                  << "', got '" << actual.generic_string() << "'\n";
        return false;
    }
}

int main()
{
    // 路径解析只处理路径字符串，不要求测试资源在磁盘上存在。
    Limen::AssetPath::SetRoot("sandbox_assets/./stale/../current");
    const bool normalizedRoot = CheckPath(
        "normalize asset root",
        Limen::AssetPath::GetRoot(),
        "sandbox_assets/current"
    );

    const bool relativePath = CheckPath(
        "resolve relative logical path",
        Limen::AssetPath::Resolve("models/./old/../cube.obj"),
        "sandbox_assets/current/models/cube.obj"
    );

    const std::filesystem::path absoluteInput =
        std::filesystem::current_path() / "external" / ".." / "outside.obj";
    const bool absolutePath = CheckPath(
        "preserve absolute path without asset root",
        Limen::AssetPath::Resolve(absoluteInput),
        std::filesystem::current_path() / "outside.obj"
    );

    const bool emptyPath = CheckPath(
        "reject empty logical path",
        Limen::AssetPath::Resolve(std::filesystem::path{}),
        std::filesystem::path{}
    );

    Limen::AssetPath::SetRoot("alternate_assets");
    const bool changedRoot = CheckPath(
        "resolve with updated asset root",
        Limen::AssetPath::Resolve("models/cube.obj"),
        "alternate_assets/models/cube.obj"
    );

    return normalizedRoot && relativePath && absolutePath &&
           emptyPath && changedRoot ? 0 : 1;
}
