//
// Created by chenlong on 2026/9/14.
//

#pragma once

#include <filesystem>

#include "Limen/Asset/Model.h"
#include "Limen/Core/Core.h"

namespace Limen
{
    /**
     * @brief 将外部模型文件转换为 Limen 的 Model 资源。
     *
     * 公共接口只出现 Limen 自己的类型，不暴露 tinyobjloader。
     * 当前第一版只支持 OBJ，后续可以根据扩展名分发到
     * glTF、FBX 等不同格式的导入实现。
     */
    class LIMEN_API ModelImporter final
    {
    public:
        // ModelImporter 当前没有实例状态，只提供静态导入入口。
        ModelImporter() = delete;

        /**
         * @brief 从指定源文件导入一个模型。
         *
         * @param sourcePath
         * OBJ 等源模型文件的路径。
         *
         * @return
         * 成功时返回可共享的 Model；
         * 文件不存在、格式不支持或解析失败时返回 nullptr。
         */
        [[nodiscard]]
        static Ref<Model> Import(
            const std::filesystem::path& sourcePath
        );
    };
}
