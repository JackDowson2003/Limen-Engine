# Mac M1 图形环境与 Limen Engine 后端规划

> 记录日期：2026-10-08。本文区分 **本机实测**、**当前仓库实现**、**API/硬件能力** 与 **尚未实现的方案**。它不是其他 M1 机器或未来 macOS 版本的保证。本文不收录序列号、Hardware UUID 等私人标识。

## 1. 本机与构建工具快照

| 项目                      | 本机结果                                                                                     | 获取方式或说明                                                                  |
|---------------------------|----------------------------------------------------------------------------------------------|---------------------------------------------------------------------------------|
| 机型                      | MacBook Pro，`MacBookPro17,1`                                                                | `system_profiler SPHardwareDataType`                                            |
| 芯片                      | Apple M1                                                                                     | 本机查询                                                                        |
| CPU                       | 8 核：4 性能核 + 4 能效核                                                                    | 本机查询                                                                        |
| GPU                       | Apple M1，8 核                                                                               | `system_profiler SPDisplaysDataType`                                            |
| 统一内存                  | 8 GB                                                                                         | 本机查询；CPU/GPU 共用这份内存，不是另有 8 GB 独立显存                          |
| 系统                      | macOS 15.6，Build `24G84`                                                                    | `sw_vers`                                                                       |
| CMake                     | 4.4.3                                                                                        | `cmake --version`；与仓库版本声明一致                                           |
| 全局 Xcode 选择           | `/Applications/Xcode14.app/Contents/Developer`，Xcode 14.1                                   | `xcode-select -p`、`xcodebuild -version`                                        |
| Limen Preset 使用的 Xcode | `/Applications/Xcode.app/Contents/Developer`，Xcode 26.2；Apple Clang 17.0.0；macOS SDK 26.2 | `CMakePresets.json` 中显式指定 `DEVELOPER_DIR` 和编译器；已在本机核对路径与版本 |
| 项目语言                  | C++20                                                                                        | 根目录 `CMakeLists.txt`                                                         |

**工具链注意：**在终端直接执行 `xcodebuild`/`xcrun` 与运行 Limen 的 CMake Preset，可能走不同 Xcode。排查编译或 Metal SDK 问题时，先确认命令的 `DEVELOPER_DIR`；不要只看全局 `xcode-select`。SDK 版本不等于运行系统版本：本机仍运行 macOS 15.6，不能因为安装了较新的 Xcode 就假定较新系统 API 可运行。

复核当前项目工具链：

```bash
cmake --version
xcode-select -p
env DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcodebuild -version
/Applications/Xcode.app/Contents/Developer/Toolchains/XcodeDefault.xctoolchain/usr/bin/clang --version
```

## 2. 当前 OpenGL 配置：已经落地的路径

### Context、Shader 与呈现

| 环节                | 当前配置                                                                      | 代码位置                                                                      |
|---------------------|-------------------------------------------------------------------------------|-------------------------------------------------------------------------------|
| 窗口与输入          | GLFW；`MacWindow` 按图形 API 选择窗口提示                                     | `LimenEngine/src/Platform/macOS/MacWindow.cpp`                                |
| OpenGL Context 请求 | **4.1 Core Profile**，`GLFW_OPENGL_FORWARD_COMPAT = true`                     | 同上，`MacWindow::Init()`                                                     |
| 函数加载            | Context 成为当前线程的当前 Context 后，通过 GLAD 加载函数                     | `LimenEngine/src/RHI/macOS/OpenGL/OpenGLContext.cpp`                          |
| Shader              | GLSL `#version 410 core`                                                      | `LimenEngine/assets/shaders/OpenGL/` 与 `LimenSandBox/assets/shaders/OpenGL/` |
| ImGui               | GLFW 平台后端 + `imgui_impl_opengl3`，传入 GLSL 410                           | `LimenEngine/src/Editor/ImGui/ImGUILayer.cpp`                                 |
| 呈现/VSync          | `glfwSwapBuffers`；`glfwSwapInterval` 可切换，创建时默认开启                  | `OpenGLContext.cpp`、`MacWindow.cpp`                                          |
| 平台编译            | CMake 在 Apple 平台加入 `src/Platform/{GLFW,macOS}` 与 `src/RHI/macOS/OpenGL` | `LimenEngine/CMakeLists.txt`                                                  |

`OpenGLContext::Init()` 会输出 `GL_VENDOR`、`GL_VERSION`、`GL_RENDERER`。本次文档编写没有保存这三个**运行时字符串**，因此不虚构驱动版本或 renderer 名称；以后以应用启动日志为准。4.1 是当前代码**请求**的 profile，成功运行只说明创建了兼容 Context；具体字符串仍应从日志核对。[Apple 的 OpenGL profile 文档](https://developer.apple.com/documentation/appkit/opengl-profiles)列出了 macOS 的 4.1 Core Profile。Apple 已废弃 macOS OpenGL，建议新开发使用 Metal；废弃不等于在本机立即不可运行。[Apple 的迁移说明](https://developer.apple.com/documentation/Apple-Silicon/porting-your-macos-apps-to-apple-silicon)

本次也没有逐项测量 `GL_MAX_TEXTURE_SIZE`、`GL_MAX_COLOR_ATTACHMENTS`、`GL_MAX_UNIFORM_BLOCK_SIZE` 或 `GL_MAX_SAMPLES` 的数值上限；需要时应在当前 Context 中用 `glGetIntegerv` 查询。已验证的 **4× MSAA 可创建** 不代表本机最大值恰好是 4，不能把某台机器的查询结果写死为引擎常量。

### 已由 Limen 使用或在本机验证

| 能力          | 当前状态与边界                                                                                                                                                                                                                                               |
|---------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| 光栅化        | 已有 Mesh、Material、GraphicsPipeline、Renderer2D、SceneRenderer、主场景与平行光 Shadow Pass。                                                                                                                                                               |
| 颜色/深度目标 | 主场景当前仍使用 `RGBA8 + Depth24Stencil8`；Shadow Map 使用 `Depth32F`。OpenGL 后端可创建单采样或 MSAA Framebuffer，并把 MSAA 颜色 Resolve 到可采样纹理。                                                                                                    |
| `RGBA16F`     | **当前工作区**已新增公共附件格式和 OpenGL 分配分支；2026-10-08 本机 Regression 按钮报告 `RGBA16F framebuffer allocation: PASS (MSAA 4x)`。这只证明单采样、4× MSAA 附件可创建且 Debug 完整性检查未失败，**没有**证明 HDR 值写入/读回，也没有启用 HDR 主场景。 |
| 纹理颜色空间  | Albedo 可用 `GL_SRGB8_ALPHA8`，采样时由 GPU 解码；Normal 等数据纹理使用 Linear 格式。当前 3D `BlinnPhong.frag` 仍在写入 `RGBA8` 前手动执行 Linear → sRGB。                                                                                                   |
| GPU 计时      | 当前 `OpenGLGPUProfiler` 使用一个覆盖整帧的 `GL_TIME_ELAPSED` 查询，并通过多帧环形槽延迟读取；具名 Scope 记录层级，但**不是逐 Scope 的 GPU 时间**。这是本项目当前实现边界，不应表述为“所有 M1 都不能做更细计时”。                                            |
| 调试扩展      | 用户在 2026-10-07 的运行日志中看到 `GL_KHR_debug: unavailable`、`GL_ARB_debug_output: unavailable`；当前 `OpenGLContext` 会重新探测并打印。不要无条件注册依赖这两个扩展的回调。                                                                              |

**当前能做：**继续实现光栅化、PBR、HDR 浮点中间目标、Tone Mapping、后处理、阴影过滤和回归测试；这些都不要求先切到 Metal。`RGBA16F` 创建成功不代表完整 HDR 链路已完成：正确流程仍需 Linear HDR 主场景 → PostProcess Tone Mapping 与显示编码 → 最终 LDR 附件。

**当前不能直接依赖：**OpenGL 4.3 Core 才纳入的 Compute Shader/Shader Storage Buffer 等能力，不能当作本项目 4.1 Core 基线；若某功能由扩展提供也必须运行时检测，不能按版本之外的能力猜测。[Khronos OpenGL 4.3 Core 规范](https://registry.khronos.org/OpenGL/specs/gl/glspec43.core.pdf) 此外，DXR、Direct3D 12、Metal 原生命令均不是 OpenGL 后端能力。GLSL 410 也不是 MSL，不能把 OpenGL Shader 原样交给 Metal。

### 本机 OpenGL 复验

在依赖已准备好的仓库根目录：

```bash
cmake --preset ninja-debug
cmake --build --preset build-debug
(cd out && ./LimenSandBox)
```

运行时查看 `Vendor`、`Version`、`Renderer` 和两个 debug 扩展的启动日志；在 `Regression` 面板点击 `Check RGBA16F framebuffer`。当前按钮只做**分配检查**，不是像素精度或视觉回归。修改 Shader/资源而未重新链接时，应按根目录 README 的资源复制步骤刷新 `out/assets`。

## 3. 本机 Metal 能力：设备支持不等于引擎已实现

以下结果是 2026-10-08 在这台机器上用 `MTLCreateSystemDefaultDevice()` 读取的 `MTLDevice` 值，**不是**从 `RendererAPI::API::METAL` 枚举推测出的引擎功能：

| Metal 设备查询                          | 本机结果   | 对 Limen 的含义                                                                                                                                                                                                                                              |
|-----------------------------------------|------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `name`                                  | `Apple M1` | 当前可选 GPU。                                                                                                                                                                                                                                               |
| `supportsFamily(.apple7)`               | `true`     | M1 对应 Apple7 家族。[Apple GPU 家族文档](https://developer.apple.com/documentation/metal/mtlgpufamily)                                                                                                                                                      |
| `supportsFamily(.mac2)`                 | `true`     | Apple silicon Mac 也报告 Mac2；不能只看其中一个家族列。[Apple 特性表](https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf)                                                                                                                        |
| `supportsFamily(.metal3)`               | `true`     | 本机 Metal 3 特性家族可用；仍需检查具体功能。[Apple Metal 3 家族文档](https://developer.apple.com/documentation/metal/mtlgpufamily/metal3)                                                                                                                   |
| `supportsRaytracing`                    | `true`     | Metal ray tracing API 可用；**不等于 M1 有专用硬件光追加速单元**。Apple 表示 Mac 上的硬件加速光追从 M3 系列开始。[Apple M3 公告](https://www.apple.com/newsroom/2023/10/apple-unveils-m3-m3-pro-and-m3-max-the-most-advanced-chips-for-a-personal-computer/) |
| `isDepth24Stencil8PixelFormatSupported` | `false`    | 未来 Metal 后端**不能直接**把当前 OpenGL 的 `Depth24Stencil8` 映射成 Metal `Depth24Unorm_Stencil8`。必须显式选兼容格式或调整公共格式契约。                                                                                                                   |
| `hasUnifiedMemory`                      | `true`     | CPU/GPU 共用物理内存；资源的 storage mode 与同步规则仍需要正确设置，不能理解为“完全没有数据传输/同步成本”。                                                                                                                                                  |
| `supportsTextureSampleCount(4)`         | `true`     | Metal 4× MSAA 可作为本机候选；管线和附件的 sample count 必须一致。其他采样数仍应逐个检查。[Apple 查询 API](https://developer.apple.com/documentation/metal/mtldevice/supportstexturesamplecount%28_%3A%29)                                                   |

复验命令（显式使用项目指向的 Xcode，避免全局 Xcode 14.1 干扰）：

```bash
env DEVELOPER_DIR=/Applications/Xcode.app/Contents/Developer xcrun swift -e '
import Metal
if let d = MTLCreateSystemDefaultDevice() {
    print("name=\(d.name)")
    print("apple7=\(d.supportsFamily(.apple7))")
    print("mac2=\(d.supportsFamily(.mac2))")
    print("metal3=\(d.supportsFamily(.metal3))")
    print("raytracing=\(d.supportsRaytracing)")
    print("depth24stencil8=\(d.isDepth24Stencil8PixelFormatSupported)")
    print("unifiedMemory=\(d.hasUnifiedMemory)")
    print("msaa4=\(d.supportsTextureSampleCount(4))")
} else {
    print("no metal device")
}'
```

### Metal 上可以规划什么

- 渲染与计算：使用 Metal 的 Render/Compute Pipeline、Command Queue/Buffer/Encoder 来承载 Limen 的主场景、阴影、后处理和计算任务；这是**未来 Metal 后端工作**，不是当前 OpenGL 路径的自动升级。
- HDR 与 MSAA：为 Linear HDR 中间目标选择浮点 `MTLPixelFormat`，为最终显示选择合适的 LDR/sRGB 输出；具体格式的渲染、采样、混合和 resolve 能力按 [Apple 特性表](https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf)及设备运行时检查，不凭 OpenGL 格式名推断。Metal 的 `_sRGB` 像素格式有读写颜色空间行为，需避免与手动编码叠加。[Apple 像素格式文档](https://developer.apple.com/documentation/metal/mtlpixelformat)
- 光追实验：`supportsRaytracing == true` 允许将 Metal ray tracing API 列为研究方向；M1 没有 M3 起提供的专用硬件光追加速，复杂实时场景的性能必须实测，不能许诺与未来 NVIDIA DXR 目标相同。
- 调试与性能：Metal 接通之后可使用 Xcode Metal Frame Capture、GPU Trace 与性能工具；**当前 OpenGL 运行不能当作 Metal 帧来捕获**。[Apple Xcode 捕获文档](https://developer.apple.com/documentation/xcode/capturing-a-metal-workload-in-xcode)

### Metal 上不能照搬什么

1. **不能把 OpenGL Context/函数当成 Metal Context/命令。**Metal 需要 `MTLDevice`、命令队列、命令缓冲、Render/Compute Encoder，以及可呈现的 drawable；`glfwSwapBuffers` 不负责呈现 Metal drawable。[Apple MetalKit 概览](https://developer.apple.com/documentation/MetalKit/MTKView)
2. **不能把 `GLuint` 纹理 ID 交给未来的 ImGui Metal 后端。**当前 `GetFinalColorAttachmentHandle()` 与 `ImGui::Image` 的整数纹理句柄是 OpenGL 阶段的实现契约；Metal 必须设计对应的非拥有纹理视图/句柄及生命周期，接入 `imgui_impl_metal` 或等价方案。
3. **不能照搬 `GL_DEPTH24_STENCIL8`。**本机 Metal `depth24Stencil8` 探测为 `false`。后续先决定公共 `Depth24Stencil8` 是“精确格式”还是“有深度和模板的语义”；若精确，Metal 应明确拒绝或新增可表达 `Depth32Float_Stencil8` 的公共格式，不能悄悄映射并假装精度未变。[Apple 设备能力 API](https://developer.apple.com/documentation/metal/mtldevice)
4. **不能假设有硬件光追、DXR 或特定 Metal 新特性。**光追 API 可用与专用硬件加速不同；OS、GPU 家族与具体 `MTLDevice` 能力须分别满足。当前系统是 macOS 15.6，本文不把更新 SDK 中出现的 Metal 4 API 当作本机运行基线。
5. **不能直接复用 GLSL。**Metal Shader Language 与 GLSL 410 的资源绑定、入口、坐标和编译流程不同；在保留 Limen 公共 Material/Shader 语义的同时，需要 Metal 专属 Shader 资源或明确的离线转换管线。

## 4. 后续 Metal 接入顺序（全部尚未实现）

| 顺序                   | 落点与责任                                                                                                                                                                                                                                                                     | 最小验证                                                                                                                   |
|------------------------|--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|----------------------------------------------------------------------------------------------------------------------------|
| 1. 设备与窗口表面      | `MacWindow.cpp` 已为 `METAL` 选择 `GLFW_NO_API`；接下来在 `GraphicsContext::Create()` 的 Metal 分支创建专属 Context/设备，借用 GLFW 的 Cocoa 窗口/视图并配置 Metal layer。`Window` 拥有窗口；Context 管理设备与呈现相关对象。                                                  | 只显示清屏色并成功 present；不动 OpenGL 默认路径。[GLFW 原生窗口访问](https://www.glfw.org/docs/latest/group__native.html) |
| 2. 构建与 Shader       | CMake 为 Metal 后端单独选择源码，按采用的窗口方案链接 Metal、QuartzCore、MetalKit 等所需 Apple frameworks，并安排 MSL 编译与资源复制；不要在公共 RHI 头暴露 `MTL*`。可评估 Objective-C++ 或 Apple 的 [metal-cpp](https://developer.apple.com/metal/cpp/)，先定所有权规则再选。 | 一个独立三角形，Shader 编译错误可定位。                                                                                    |
| 3. RHI 基础资源        | 逐个实现 Buffer、Texture、Framebuffer、RenderPass、GraphicsPipeline、Draw 命令；按设备能力处理 4× MSAA、深度/模板与 HDR 格式，正确安排 encoder、resolve 和资源状态。                                                                                                           | 主场景与 Shadow Pass 分阶段和 OpenGL 对照。                                                                                |
| 4. SceneRenderer 与 UI | 仍由 `SceneRenderer` 组织 Shadow/Main/PostProcess；Metal 后端实现资源和命令。接入 ImGui Metal renderer，并让最终显示纹理生命周期覆盖 UI 提交。                                                                                                                                 | 同一固定场景和光照参数的画面对照，颜色空间一致。                                                                           |
| 5. 计时与光追研究      | Metal 专属 GPU Profiler/捕获；Ray tracing 仅在运行时检查通过后作为可选研究路径。                                                                                                                                                                                               | 帧时间、资源使用量、视觉结果分别测量，不以“API 可用”替代性能结论。                                                         |

公共调用链仍应是 `Scene → SceneRenderer → RenderPass/Renderer → RHI → 选中的后端`。`Scene` 不发 GPU 命令，`SceneRenderer` 管理渲染阶段，Metal/OpenGL 对象只属于各自私有后端。`Application::~Application()` 当前先销毁 Layer 和资产资源，再关闭 Renderer，最后销毁窗口/Context；Metal 也必须保证 GPU 工作完成或安全回收后，才释放依赖设备/Drawable 的资源。

## 5. 本文与 README 的边界

README 仍把“无 `RGBA16F`”和“无 GPU Profiler”列为限制；**当前工作区代码**已加入 OpenGL `RGBA16F` 附件创建与整帧 GPU 计时，因此这两句对当前工作区不再完全准确。真正仍未完成的是 **Linear HDR 主场景、PostProcess/Tone Mapping、逐 Scope GPU 时间与 Metal 后端**。本文不顺手修改 README、CMake、`.gitmodules`、Shader 或现有未提交代码；后续合并这些改动时再统一更新 README。

## 6. 参考资料

- [Apple：macOS OpenGL Profiles](https://developer.apple.com/documentation/appkit/opengl-profiles)；[Apple：OpenGL 在 Apple silicon 上的状态](https://developer.apple.com/documentation/Apple-Silicon/porting-your-macos-apps-to-apple-silicon)。
- [Khronos：OpenGL 4.3 Core Specification](https://registry.khronos.org/OpenGL/specs/gl/glspec43.core.pdf)：核对超过当前 4.1 Core 基线的功能。
- [Apple：Metal GPU 家族](https://developer.apple.com/documentation/metal/mtlgpufamily)；[Metal Feature Set Tables](https://developer.apple.com/metal/Metal-Feature-Set-Tables.pdf)：功能与像素格式的官方对照，仍以本机 `MTLDevice` 实测为准。
- [Apple：M3 系列首次为 Mac 带来硬件加速光追](https://www.apple.com/newsroom/2023/10/apple-unveils-m3-m3-pro-and-m3-max-the-most-advanced-chips-for-a-personal-computer/)；[Apple：Xcode Metal 帧捕获](https://developer.apple.com/documentation/xcode/capturing-a-metal-workload-in-xcode)。
- [Apple：metal-cpp](https://developer.apple.com/metal/cpp/)；[GLFW：Native Access](https://www.glfw.org/docs/latest/group__native.html)。
