# Limen Engine 正式 Editor：C++、C# 与 Python 路线对比

> 状态：技术选型记录。正式 Editor 已选择 C++ 自研 UI，C# 计划用于游戏脚本 1.0，不用于 Editor UI；`LimenEditor` 独立空壳已通过启动验证。以下三路线比较保留为决策依据，不表示 UI 选型仍开放。核对日期：2026-10-14。

## 1. 结论与决策范围

Limen 的渲染核心继续使用 C++。正式 Editor 已选择 C++ 自研 UI；这项选择主要影响**窗口和视口如何集成、场景数据如何编辑、UI 系统由谁维护**，不直接决定 Shader 画质，也不能仅凭语言推断场景帧率。

- **A：C++ 自研 UI（已选）**：把 UI 基础设施作为长期建设目标，同时接受布局、文本、输入和停靠等系统的持续维护成本；选择它不是因为 C++ 天然更美观或 C# 性能不足。
- **B：C# 桌面 Editor（未选）**：若不自研桌面 UI，本可借助现成控件建设工作流，但需要解决跨语言接口和原生视口集成。
- **C：Python 桌面 Editor（未选）**：可用于原型、自动化和工具脚本；是否胜任完整 Editor 取决于具体框架和同等条件下的验证，不能只凭语言推断。
- **现有 ImGui**：仅保留为 Sandbox 调试与实验基线，不承担正式 Editor UI；Qt 也不在当前路线内。

路线虽已确定，仍需验证 Editor 数据基础、视口、输入与资源生命周期。下面的比较解释取舍，不替代已选路线的实现验收。

## 2. 当前 Limen 的事实基线

| 事实                                                                                        | 当前代码证据                                                                                         | 对选型的影响                                                                                   |
|---------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|
| `Application` 创建窗口、初始化 Renderer/AssetManager；仅在客户端启用时安装 ImGui Layer      | [`Application.cpp`](../LimenEngine/src/Application/Application.cpp) 构造函数                         | Sandbox 显式启用调试层；Editor 空壳使用默认的关闭状态。                                        |
| `Application::Run()` 轮询事件、更新 Layer、结束渲染并 Present；ImGui 调用受调试层存在性控制 | 同文件 `Run()`                                                                                       | 正式 C++ UI 尚未接入帧循环，不能将 ImGui 回调当成它的绘制阶段。                                |
| `Example3DLayer` 在 `OnUpdate()` 渲染场景，在 `OnImGuiRender()` 记录视口尺寸并显示最终纹理  | [`Example3DLayer.cpp`](../LimenSandBox/src/Example3DLayer.cpp)                                       | 场景渲染可复用；视口尺寸、焦点、悬停及调试 UI 必须从 ImGui 回调中解耦。                        |
| `SceneRenderer` 提供 `Render()`、`Resize()` 和最终颜色附件借用句柄                          | [`SceneRenderer.h`](../LimenEngine/include/Limen/Scene/SceneRenderer.h)                              | 当前 OpenGL 返回的句柄是纹理 ID，**不是**可跨进程或跨图形 API 直接传输的图像；调用者不拥有它。 |
| `SceneRenderObjectHandle` 当前只含数组下标                                                  | [`Scene.h`](../LimenEngine/include/Limen/Scene/Scene.h)                                              | 正式 Hierarchy、删除、撤销和跨语言引用需要更可靠的对象身份机制。                               |
| 引擎默认是静态库，只有可选的共享库构建开关                                                  | [`CMakeLists.txt`](../CMakeLists.txt)、[`LimenEngine/CMakeLists.txt`](../LimenEngine/CMakeLists.txt) | 开启共享库不等于已经有 C# 或 Python 可用的稳定接口。                                           |
| `LimenEditor` 已有独立应用空壳，但无正式 UI；序列化、AssetID 尚未完成                       | [`EditorApp.cpp`](../Limen-Editor/src/EditorApp.cpp)、[`README.md`](../README.md) 的阶段 6–8         | 空窗口验证的是应用边界，不是场景编辑或多窗口能力。                                             |

Sandbox 的帧链路可概括为：`GLFW 事件 → Application::Run → Layer::OnUpdate → SceneRenderer → 离屏颜色纹理 → ImGui::Image → Present`。Editor 空壳目前没有场景 Layer 和 ImGui，运行的是清屏与 Present；独立进程能启动并不证明正式 UI 已完成。

## 3. 三条方案的具体比较

以下的“高/低”是相对 Limen **当前代码**的工程判断，不是跑分或交付工期。

| 维度                  | A：C++ 自研、由引擎渲染的 UI                                                        | B：C# 桌面 Editor + C++ 引擎                                                                   | C：Python 桌面 Editor + C++ 引擎                                                         |
|-----------------------|-------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------------|------------------------------------------------------------------------------------------|
| 场景与资源访问        | 同语言，能调用现有 C++ 接口；仍应经过 Editor 命令层，避免 UI 直接改坏 Scene。       | 需要稳定的原生接口、对象句柄、数据转换和错误约定；不能直接暴露 `Ref<Mesh>` 等 C++ 类型。       | 同样需要原生接口或 Python 扩展，以及句柄、所有权和异常边界。                             |
| 窗口与视口            | 可沿当前原生窗口/图形 Context 演进；还要自行解决 UI 绘制、输入和合成。              | 桌面 UI 框架可管理面板；内嵌原生视口、图形 Context、焦点、输入和高 DPI 需要平台集成。          | 可选择支持原生 OpenGL 画布的框架，但当前 OpenGL 路径不能直接代表未来 Windows/DX12 路径。 |
| UI 基础设施           | 布局、文本/字体、输入法、键盘焦点、停靠、拖放、主题等由项目持续建设和测试。         | 借助桌面 UI 框架已有的控件、主题与数据绑定；仍需定制引擎视口和工作流。                         | 借助所选 Python UI 框架；能力、外观和视口集成取决于**具体框架**，不能由语言推断。        |
| Editor 与游戏 UI 复用 | 若从一开始设计为引擎 UI，可共享绘制、样式和输入基础；Editor 专属面板仍留在 Editor。 | 桌面框架的控件不会自动成为游戏内 UI；可共享部分 C# 数据模型/工具代码，但游戏 UI 仍需单独设计。 | 同左；Python 编辑器界面也不会自动提供可发布的游戏内 UI。                                 |
| 渲染性能              | 场景渲染仍由 Limen C++/GPU 负责。                                                   | 同左；开销主要取决于视口呈现方式和跨语言调用粒度，而不是 C# 这个名字。                         | 同左；大量逐对象、逐帧 Python 工作更需要测量，但不能预设固定帧率差。                     |
| 美观与交互品质        | 完全可控，设计与基础控件也完全自负。                                                | 可使用主题和模板快速形成一致外观；最终质量仍取决于设计与实现。                                 | 同样能做定制外观；具体上限与成本取决于框架和设计投入。                                   |
| 调试、发布与维护      | 一套主要 C++ 构建链；UI 框架本身成为长期维护责任。                                  | 增加 .NET 构建/运行时与原生库打包；跨边界错误需要双侧诊断。                                    | 增加 Python 运行时、依赖及原生扩展打包；跨边界错误同样需要双侧诊断。                     |
| 当前最大的未知量      | 自研 UI 在文本、输入、布局和多窗口下能否长期保持质量与可维护性。                    | macOS 和未来 Windows 的内嵌 GPU 视口能否稳定、低开销地工作。                                   | 选定框架在复杂桌面面板和未来 DX12 视口上的实际可维护性。                                 |

### A. C++ 自研 UI：收益来自“系统复用”，不是语言速度

Unreal 的 Slate 是为 Editor 和游戏 UI 建设的跨平台 UI 框架；Godot 的 Editor 也使用引擎自己的渲染和 UI 系统。这些是**架构实例**，不是 Limen 已经具备相同基础设施的证明。[Unreal Slate](https://dev.epicgames.com/documentation/unreal-engine/slate-overview-for-unreal-engine?lang=en-US) · [Godot Editor](https://docs.godotengine.org/en/stable/engine_details/editor/introduction_to_editor_development.html)

对 Limen 的潜在收益是单一的窗口/输入/资源生命周期、视口与 UI 的统一合成，以及未来 Editor/游戏运行时复用。但目前 `Renderer2D` 和 ImGui 调试面板并不等于完整 UI 框架。若要达到正式 Editor 品质，还需要解决字体与输入法、高 DPI、布局、裁剪、焦点、快捷键、拖放、停靠、可访问性及大量控件的测试。自研不等于每个底层算法都必须自己写；是否采用辅助库应单独评估。

**已接受的前提：**将 UI 基础设施作为长期建设内容，并持续投入维护；仍须控制投入，避免挤占渲染和场景系统的基础工作。

### B. C# 桌面 Editor：快建面板，难在引擎边界

以 Avalonia 为**待验证候选**而非已选依赖：其主题、数据绑定可用于 Inspector、Hierarchy、Content Browser；`NativeControlHost` 可承载原生视图，但原生视图的层叠和覆盖有限制，不能假定现有 GLFW 窗口可无改动嵌入。[Avalonia 主题](https://docs.avaloniaui.net/docs/styling/themes) · [Avalonia 原生视图](https://docs.avaloniaui.net/docs/app-development/native-interop)

拟议架构是“C# UI 发送较粗粒度的编辑命令 → C++ 拥有 Scene/资源并渲染 → 原生视口呈现 GPU 结果”。跨语言接口应使用明确的句柄和值数据，约定创建/销毁、UTF-8 字符串、错误传递和线程；不能把现有 C++ 模板/智能指针直接作为 ABI。微软的 .NET 文档建议通过 C ABI 与 C++ 互操作。[.NET ABI 文档](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/abi-support)

**适用前提：**Editor 的价值主要在项目、资产、场景工作流，而不是自研 UI 技术；并且原生视口 PoC 能通过下文验收。C# Editor 可以与未来 C# 游戏脚本共享部分 SDK 和工具代码，**但不会自动实现 Player 中的 C# 运行时**。

### C. Python 桌面 Editor：不能只因原型速度而定案

Python 可以通过 `ctypes` 调用共享库；例如 wxPython 提供 `GLCanvas`，因此“Python 不能显示实时 OpenGL 视口”并不成立。但 `GLCanvas` 有自己的 Context 与高 DPI 规则，未来 DX12 也仍需另外设计视口接入。[Python `ctypes`](https://docs.python.org/3/library/ctypes.html) · [wxPython `GLCanvas`](https://docs.wxpython.org/wx.glcanvas.GLCanvas.html)

Python 更适合当前阶段的自动化、离线资产工具和快速验证。若把它作为完整 Editor 主语言，应与 C# 方案进行**同功能**的视口、列表规模、输入响应、打包和崩溃诊断测试，而不是预先断言它一定慢或一定不好看。

## 4. 无论选择哪条路线，都必须守住的边界

1. **所有权与生命周期：**Scene、AssetManager、SceneRenderer 和 GPU 资源由 C++ 引擎管理。UI 可以借用结果，不能私自释放 GPU 附件；先销毁使用 Context 的资源，再销毁 Context/窗口。当前销毁顺序见 [`Application.cpp`](../LimenEngine/src/Application/Application.cpp)。
2. **Editor 数据操作：**未来的 Editor 命令/撤销层应验证对象身份与操作前提。当前只有数组下标句柄；安全删除、Generation Handle、场景序列化和 AssetID 仍是待做能力，见 [`README.md`](../README.md) 阶段 6。这里的“Editor 命令层”是**拟议职责**，不是仓库里已有的类。
3. **视口与 RHI：**当前 `GetFinalColorAttachmentHandle()` 返回借用的 OpenGL 纹理 ID。它只在相应资源仍存活、图形 Context 正确的情况下有意义；不能把整数本身当成跨进程图像，也不能把 `GLuint` 写进公共 Scene/RHI 接口。未来 DX12 需要后端对应的呈现路径。
4. **性能：**不要把每帧视口图像读回 CPU 再交给 C#/Python 绘制作为默认架构；应优先验证原生 GPU 视口。跨语言调用避免逐像素、逐物体的高频往返，尽量传编辑命令或批量快照。是否达标以实测为准。
5. **脚本独立：**已选择 C++ 自研 Editor UI，并计划在游戏脚本 1.0 中支持 C#；两者是独立系统，完成 Editor 不会自动实现 Player 中的 C# 运行时。
6. **当前路线：**`README.md` 把 Entity/序列化/AssetID 列在正式 Editor 前。可以提前做技术验证，但不应把 PoC 误写成可完整编辑、保存和发布项目的 Editor。

## 5. 如何做公平的技术验证

先固定同一测试场景、同一视口物理像素尺寸与相同 VSync 状态，记录当前 Sandbox 的 CPU 帧时间和 OpenGL GPU 时间作为**基线**。验证已选 C++ UI 路线时至少检查以下项目；若未来复评其他路线，沿用同一标准：

| 检查项       | 通过条件                                                                                                                 |
|--------------|--------------------------------------------------------------------------------------------------------------------------|
| 实时视口     | 场景由原 C++/GPU 管线渲染；无每帧同步 CPU 像素读回；视口缩放与高 DPI 下不变形。                                          |
| 输入与交互   | 焦点、鼠标移动/滚轮、键盘快捷键在视口和面板之间切换正确；不会被 UI 误吞或重复投递。                                      |
| 资源生命周期 | 反复打开/关闭、调整窗口、切换场景后无无效纹理句柄；退出时 GPU 资源先于 Context 释放。                                    |
| 编辑数据     | 同一最小场景可在 Hierarchy 选中物体、在 Inspector 修改 Transform；变化经过同一可验证的编辑操作，不由 UI 直接改内部数组。 |
| 性能         | 比较相同场景的 CPU/GPU 帧时间分布、视口输入延迟与 UI 滚动响应；先测基线再定阈值，不虚构“语言导致的 FPS 差”。             |
| 扩展性       | 评审将来 Windows/DX12 的视口显示和打包路线；不能以当前 M1/OpenGL PoC 当作跨平台证明。                                    |

PoC 不要求一开始就完成全部正式 Editor 功能。已选 C++ 路线仍需先证明**视口、输入、对象身份与生命周期**这些最难逆转的边界；不能因为已有空窗口，就把 UI、停靠或场景编辑记为完成。

## 6. 已选方案与成本边界

- **正式 Editor UI：**选择 A，即 C++ 自研 UI；先从单窗口绘制、输入和布局基础做可验证的增量，再扩展到多窗口与停靠面板。
- **游戏脚本 1.0：**计划支持 C#；脚本宿主与 Editor UI 分开设计和验收。
- **未选方案：**B、C 的比较作为历史决策依据保留，不作为当前 Editor 的并行实现任务。

[`README.md`](../README.md) 中正式 Editor 的“20–40 个有效开发日第一版、3–6 个月稳定版”是旧范围的粗估，**不能直接当作自研完整 UI 框架的工期**；应在首个可运行的 UI/视口 PoC 后重估。当前只创建了可启动的 `LimenEditor` 空壳，没有正式 UI、场景编辑或多窗口能力；本文不引入新依赖，也不改变渲染架构。

## 参考资料

- [Unreal Engine：Slate Overview](https://dev.epicgames.com/documentation/unreal-engine/slate-overview-for-unreal-engine?lang=en-US)：自研跨平台 UI 用于工具与游戏 UI 的实例。
- [Godot：Introduction to editor development](https://docs.godotengine.org/en/stable/engine_details/editor/introduction_to_editor_development.html)：编辑器使用引擎自身渲染/UI 系统，以及 Editor → Scene → Core 的依赖边界。
- [Microsoft：Native interoperability ABI support](https://learn.microsoft.com/en-us/dotnet/standard/native-interop/abi-support)：.NET 与 C++ 的 ABI 边界。
- [Avalonia：Native platform interop](https://docs.avaloniaui.net/docs/app-development/native-interop)：原生视图嵌入与层叠限制。
- [Python：`ctypes`](https://docs.python.org/3/library/ctypes.html)、[wxPython：`GLCanvas`](https://docs.wxpython.org/wx.glcanvas.GLCanvas.html)：Python 调用原生库与 OpenGL 画布的可行性及边界。
