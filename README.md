# Limen Engine

Limen Engine 是一个用于学习并逐步实现现代实时渲染架构的 C++ 游戏引擎项目。

当前可运行的平台与图形 API 组合是：

| 操作系统 | 图形 API        | 当前状态             |
|----------|-----------------|----------------------|
| macOS    | OpenGL 4.1      | 已实现，当前默认后端 |
| macOS    | Metal           | 规划中               |
| Windows  | Direct3D 11     | 规划中.3             |
| Windows  | Direct3D 12     | 规划中               |
| Linux    | OpenGL / Vulkan | 规划中               |
| IOS      | Metal           | 规划中               |
| Andriod  | Vulkan          | 规划中               |

Windows 版本的 Limen Engine 将只支持 Direct3D 11/12，不提供 Windows OpenGL 后端。macOS 当前使用 OpenGL 完成引擎基础功能，后续再加入原生 Metal 后端。

## 获取与构建

项目使用 Git submodule 保存部分第三方依赖。克隆仓库后执行：

```bash
git submodule update --init --recursive
```

macOS Debug 构建：

```bash
cmake --preset build-debug
cmake --build --preset build-debug
```

当前可执行文件生成到：

```text
out/LimenSandBox
```

## 架构原则

项目把“高层渲染逻辑”“RHI 抽象”“具体图形 API 后端”和“操作系统平台代码”分开存放。

```text
LimenSandBox / 游戏代码
           ↓
Renderer 高层渲染接口
           ↓
RHI 公共资源与命令抽象
           ↓
macOS/OpenGL 后端实现
           ↓
OpenGL / GLAD / GLFW
```

操作系统窗口是另一条依赖链：

```text
Application
    ↓
Window 公共接口
    ↓
MacWindow
    ↓
GLFW / macOS
```

这两条链在创建图形上下文时组合：

```text
macOS + OpenGL
    = MacWindow + OpenGLContext + OpenGLRendererAPI
```

以后加入 Metal 时仍然使用 `MacWindow`，只把 Context、RendererAPI 和资源实现替换为 Metal 版本，不创建 `MacOpenGLWindow` 或 `MacMetalWindow`。

## 当前目录结构

```text
Limen-Engine/
├── LimenEngine/
│   ├── include/
│   │   ├── Limen.h                         # Sandbox 使用的总入口头文件
│   │   └── Limen/
│   │       ├── Application/                # Application、Window、Layer
│   │       ├── Core/                       # 基础宏、智能指针别名、日志、DeltaTime
│   │       ├── Events/                     # 窗口、键盘、鼠标事件
│   │       ├── Input/                      # 跨平台输入接口与统一键码
│   │       ├── Renderer/                   # Renderer 前端与 Camera
│   │       ├── RHI/                        # Buffer、Shader、Texture 等公共抽象
│   │       ├── EntryPoint.h                # 客户端程序入口
│   │       └── lmpch.h                     # 仅引擎内部使用的预编译头
│   │
│   ├── src/
│   │   ├── Application/                    # Application/Layer/Window 的实现
│   │   ├── Core/                           # 日志等核心实现
│   │   ├── Renderer/                       # 高层场景提交与相机实现
│   │   ├── RHI/
│   │   │   ├── Common/                     # RHI 工厂和 API 无关实现
│   │   │   └── macOS/
│   │   │       └── OpenGL/                 # 当前唯一完成的图形 API 后端
│   │   ├── Platform/
│   │   │   ├── GLFW/                       # GLFW 输入与统一键码转换
│   │   │   └── macOS/                      # MacWindow
│   │   └── Editor/
│   │       └── ImGui/                      # 引擎私有的 ImGui Layer
│   │
│   └── vendor/                             # GLFW、GLAD、GLM、ImGui、spdlog、stb_image
│
├── LimenSandBox/
│   ├── assets/                             # 测试纹理与 Shader
│   └── src/                                # 客户端示例 Layer
│
├── cmake/                                  # CMake 辅助脚本
├── CMakeLists.txt
└── CMakePresets.json
```

## 各层职责

### Core

`Core` 只能保存绝大多数模块都会使用的基础设施，例如：

- `Scope`、`Ref` 及其创建函数；
- DLL/静态库导出宏；
- Assert 和 DebugBreak；
- 日志；
- `DeltaTime`。

不要把 OpenGL、GLFW、Camera 或 Renderer 代码放入 Core。

### Application

`Application` 负责程序生命周期，而不是具体渲染算法：

- 创建和销毁 Window；
- 管理 LayerStack；
- 每帧 PollEvents、Update、ImGui、Present；
- 分发窗口和输入事件。

### Renderer

`Renderer` 是客户端面对的高层渲染入口：

- `BeginScene()` 保存相机和场景信息；
- `Submit()` 提交要绘制的对象；
- `EndScene()` 结束逻辑提交区间；
- Camera 负责 View、Projection 和 ViewProjection。

Renderer 不应该长期依赖 `gl*`、`ID3D12*` 或 Metal 原生类型。

### RHI

RHI（Rendering Hardware Interface）隔离不同图形 API。

公共 RHI 接口位于 `include/Limen/RHI`：

- `RendererAPI`；
- `Buffer`；
- `VertexArray`；
- `Shader`；
- `Texture`。

工厂和公共实现位于 `src/RHI/Common`，具体 OpenGL 实现位于 `src/RHI/macOS/OpenGL`。

例如：

```text
VertexBuffer             公共抽象
    ↓ factory
OpenGLVertexBuffer       macOS/OpenGL 实现
```

### Platform

`Platform` 只处理窗口、输入和操作系统差异。

- `Platform/macOS` 保存 `MacWindow`；
- `Platform/GLFW` 保存 GLFW 输入查询和键码转换；
- OpenGL Buffer、Shader、Texture 不属于 Platform，而属于 RHI 后端。

### Editor/ImGui

`ImGUILayer` 由 `Application` 内部创建，客户端只实现 `Layer::OnImGuiRender()`。

因此 `ImGUILayer.h` 是引擎私有头文件，位于 `src/Editor/ImGui`，不会通过 `Limen.h` 暴露给 Sandbox。

## Public 与 Private 规则

`include/Limen` 中的头文件属于公共 API，Sandbox 可以包含：

```cpp
#include "Limen/Application/Layer.h"
#include "Limen/Renderer/PerspectiveCamera.h"
#include "Limen/RHI/Texture.h"
```

`src` 中的头文件属于引擎私有实现，原则上只有 LimenEngine 自己可以包含：

```text
MacWindow.h
GraphicsContext.h
OpenGLShader.h
OpenGLRendererAPI.h
ImGUILayer.h
```

公共头文件必须自包含：如果头文件使用 `std::vector`，它自己就必须 `#include <vector>`，不能依赖预编译头或其他文件偶然包含。

## 初始化与每帧调用链

初始化：

```text
CreateApplication
    ↓
Application 选择 RendererAPI
    ↓
Window::Create → MacWindow
    ↓
GraphicsContext::Create → OpenGLContext
    ↓
Renderer::Init → OpenGLRendererAPI
    ↓
ImGUILayer::OnAttach → GLFW + OpenGL3 ImGui backend
```

每帧：

```text
PollEvents
    ↓
Layer::OnUpdate
    ↓
ImGUILayer::Begin
    ↓
Layer::OnImGuiRender
    ↓
ImGUILayer::End
    ↓
Present
```

## CMake 后端边界

当前 CMake 只收集：

- 平台无关的 Core/Application/Renderer/RHI Common；
- `Platform/GLFW`；
- `Platform/macOS`；
- `RHI/macOS/OpenGL`。

这样未来添加 Windows 文件时，Windows 构建不会意外编译 macOS/OpenGL 后端。尚未实现的平台会在 CMake 配置阶段明确失败，而不是生成一个缺少实现的程序。

## 后续扩展位置

Metal 后端加入：

```text
src/RHI/macOS/Metal/
├── MetalContext.h
├── MetalContext.mm
├── MetalRendererAPI.h
├── MetalRendererAPI.mm
├── MetalBuffer.*
├── MetalShader.*
└── MetalTexture2D.*
```

Windows 后端加入：

```text
src/Platform/Windows/
├── WindowsWindow.*
└── WindowsInput.*

src/RHI/Windows/
├── D3D11/
└── D3D12/
```

最终允许的运行时选择是：

```text
macOS   → OpenGL 或 Metal
Windows → D3D11 或 D3D12
```

不允许 Windows 自动选择或回退到 OpenGL。选择了尚未编译的后端时必须明确报错。

## 当前过渡期事项

目前 Sandbox 和 `Renderer::Submit()` 仍有少量代码直接向下转换为 `OpenGLShader`，用于上传 OpenGL Uniform。这属于当前 OpenGL 学习阶段的过渡实现。

后续加入 Material、UniformBuffer/ConstantBuffer 和 Shader 参数系统后，应删除高层代码对 `OpenGLShader` 的直接依赖，使 Renderer 只操作跨 API 的资源和参数接口。
