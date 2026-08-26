//
// Created by chenlong on 2026/8/11.
//
#pragma once

#include <filesystem>
#include <string>
#include <unordered_map>

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

#include "Limen/Core/Core.h"

namespace Limen
{
    class LIMEN_API Shader
    {
    public:
        virtual ~Shader() = default;

        virtual void Bind() const = 0;

        virtual void UnBind() const = 0;

        /**
         * @brief 设置一个 4×4 矩阵参数。
         *
         * @param name Shader 中声明的参数名称。
         * @param matrix 要上传的矩阵。
         */
        virtual void SetMat4(const char *name, const glm::mat4 &matrix) = 0;

        /**
         * @brief 设置一个三维浮点向量参数。
         *
         * @param name Shader 中声明的参数名称。
         * @param value 要上传的三维向量。
         */
        virtual void SetFloat3(const char *name, const glm::vec3 &value) = 0;

        /**
         * @brief 设置一个整数 Shader 参数。
         *
         * 目前主要用于设置纹理采样器所使用的纹理槽。
         *
         * @param name Shader 参数名称，例如 u_Texture。
         * @param value 整数值，例如纹理槽 0。
         */
        virtual void SetInt(
            const char* name,
            int value
        ) = 0;

        virtual void SetInt(
            const char* name,
            uint32_t value
        ) = 0;

        /**
         * @brief 将 Shader 中的 Uniform Block 关联到指定绑定点。
         *
         * OpenGL 对应 glUniformBlockBinding()；Direct3D 后端将映射到
         * Constant Buffer 的绑定位置。
         *
         * @param blockName Shader 中的 Uniform Block 名称。
         * @param binding UniformBuffer 使用的绑定点。
         */
        virtual void SetUniformBufferBinding(
            const char* blockName,
            uint32_t binding
        ) = 0;


        /**
         * @brief 获取Shader的逻辑名称。
         *
         * 该名称用于ShaderLibrary查找，不代表底层GPU对象ID。
         */
        [[nodiscard]]
        virtual const std::string &GetName() const noexcept = 0;

        /**
         * @brief 从内存中的源码字符串创建Shader。
         *
         * @param name Shader的逻辑名称，之后可作为ShaderLibrary中的查找键。
         * @param vertexSource Vertex Shader的完整源码。
         * @param fragmentSource Fragment Shader的完整源码。
         *
         * @return 创建成功时返回Shader共享引用，否则返回nullptr。
         */
        [[nodiscard]]
        static Ref<Shader> CreateFromSource(
            const std::string &name,
            const std::string &vertexSource,
            const std::string &fragmentSource
        );
        /**
         * @brief 从两个Shader源码文件创建一个图形Shader Program。
         *
         * 该函数负责：
         *
         * 1. 读取Vertex Shader文件；
         * 2. 读取Fragment Shader文件；
         * 3. 把读取到的源码交给当前Renderer后端编译和链接。
         *
         * @param vertexPath
         * Vertex Shader源码文件路径。
         * OpenGL阶段通常使用.vert后缀。
         *
         * @param fragmentPath
         * Fragment Shader源码文件路径。
         * OpenGL阶段通常使用.frag后缀。
         *
         * 普通相对路径会优先拼接assets/shaders目录；若拼接后的文件不存在，
         * 则回退到调用者传入的原始相对路径。为了兼容旧代码，shaders/...
         * 和assets/shaders/...也可以传入。Shader逻辑名称自动从vertexPath
         * 的文件主名提取，例如BlinnPhong.vert得到BlinnPhong。
         *
         * @return
         * 创建成功时返回Shader的共享引用；
         * 文件读取或Shader创建失败时返回nullptr。
         */
        [[nodiscard]]
        static Ref<Shader> CreateFromFiles(
            const std::filesystem::path &vertexPath,
            const std::filesystem::path &fragmentPath
        );

        /**
         * @brief 从两个Shader文件创建Shader，并显式指定逻辑名称。
         *
         * @param name ShaderLibrary中使用的名称；为空时从vertexPath.stem()提取。
         * @param vertexPath Vertex Shader文件路径。
         * @param fragmentPath Fragment Shader文件路径。
         */
        [[nodiscard]]
        static Ref<Shader> CreateFromFiles(
            std::string name,
            const std::filesystem::path &vertexPath,
            const std::filesystem::path &fragmentPath
        );

        /**
         * @brief 旧的源码创建入口，仅用于过渡期兼容。
         *
         * @deprecated
         * 请根据输入类型改用CreateFromSource()或CreateFromFiles()。
         *
         * [[deprecated]]是标准Cpp 14以上支持的，Clang、GCC和MSVC都支持。
         */
        [[nodiscard]]
        [[deprecated(
            "This function is deprecated, use Shader::CreateFromSource() or Shader::CreateFromFiles() instead"
        )]]
        static Ref<Shader> Create(
            const std::string &name,
            const std::string &vertexSource,
            const std::string &fragmentSource
        );
    };

    /**
     * @brief 用于管理Shader 并负责创建按照文件路径创建Shader
     *
     * 会进行Shader的缓存
     */
    class LIMEN_API ShaderLibrary
    {
    public:
        /**
         * @brief 将已创建的Shader加入库中。
         *
         * @param shader 要共享和缓存的Shader，不能为nullptr。
         */
        void Add(const Ref<Shader> &shader);

        /**
         * @brief 根据当前RendererAPI加载图形Shader。
         *
         * 调用者只传不带后端目录和阶段扩展名的逻辑路径：
         *
         *     Load("Example3D/BlinnPhong")
         *
         * OpenGL会解析为OpenGL目录下的.vert和.frag文件；Direct3D 11/12
         * 会解析为DirectX11/DirectX12目录下的.vs.hlsl和.ps.hlsl文件。
         * Metal与Vulkan也保留各自的目录和扩展名规则。
         *
         * 逻辑路径同时作为缓存键，因此不同目录可以拥有同名Shader。
         *
         * @param logicalPath 相对于后端Shader目录的路径，不包含扩展名。
         */
        [[nodiscard]]
        Ref<Shader> Load(const std::filesystem::path &logicalPath);

        /**
         * @brief 从两个阶段文件加载Shader。
         *
         * Shader名称自动从vertexPath的文件主名提取；若该名称已经存在，
         * 直接复用库中的Shader，不重新读取、编译和链接。
         */
        [[nodiscard]]
        Ref<Shader> Load(
            const std::filesystem::path &vertexPath,
            const std::filesystem::path &fragmentPath
        );

        /**
         * @brief 使用显式名称从两个阶段文件加载Shader。
         *
         * @param name ShaderLibrary中的查找键；为空时自动从vertexPath提取。
         * @param vertexPath Vertex Shader文件路径。
         * @param fragmentPath Fragment Shader文件路径。
         */
        [[nodiscard]]
        Ref<Shader> Load(
            std::string name,
            const std::filesystem::path &vertexPath,
            const std::filesystem::path &fragmentPath
        );

        /**
         * @brief 根据逻辑名称获取已加载Shader。
         *
         * @return 找到时返回共享引用，否则返回nullptr并记录错误。
         */
        [[nodiscard]]
        Ref<Shader> Get(const std::string &name) const;

    private:
        std::unordered_map<std::string, Ref<Shader> > m_Shaders;

        /**
         * @brief 检查指定名称是否已经加载。
         *
         * @param name 要查询的Shader逻辑名称。
         * @return 已加载时返回true，否则返回false。
         */
        [[nodiscard]]
        bool Exists(const std::string &name) const noexcept;
    };
}
