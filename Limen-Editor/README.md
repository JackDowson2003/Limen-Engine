# Limen Editor

> 状态：`LimenEditor` 独立应用空壳已通过启动验证；正式 Editor 选择 C++ 自研 UI，C# 计划用于游戏脚本 1.0，不用于 Editor UI。场景编辑、UI 面板与多窗口尚未实现。

## 已确认的架构规则

1. **独立应用：**Limen Editor 与 `LimenSandBox` 是两个客户端，分别使用 C++ 引擎核心；正式功能不继续堆进 Sandbox，Sandbox 保留为渲染实验和回归入口。
2. **一个会话、多窗口：**同一个 Editor 会话可以管理主窗口和多个浮动原生窗口。窗口是面板的承载位置，不各自创建一份场景。一个统一的 UI/渲染调度者管理这些窗口；这不等于只能打开一个窗口。
3. **面板与窗口分离：**Scene 视口、Content Browser、Outliner 和 Details 是可停靠的面板。面板可以在主窗口内重排，也可以拖出成为浮动窗口，再重新停靠。默认布局参考 UE 的工作流，但不照搬其图标、样式或源码。
4. **共享编辑状态：**同一会话中的 Outliner、Details 和各窗口读取同一个活动场景与选择状态。拖动或关闭浮动窗口只影响窗口及其中的面板，不复制或销毁场景；重新打开面板后仍能访问该会话。
5. **区分数据所有权：**实际 `Scene`、Asset 和 GPU 资源由 C++ 引擎侧管理。Editor 会话管理当前场景的使用与编辑状态；窗口和普通面板只借用这些数据，不拥有或释放场景。`SceneRenderer` 渲染时只读取 `Scene`，不负责 Editor 窗口和停靠布局。
6. **视口有自己的视图状态：**Scene 视口面板可以拥有自己的相机、尺寸和显示状态，但引用同一活动场景。多个视口是否各自拥有 `SceneRenderer`、如何共享图形资源和处理 Context 生命周期，需在视口设计阶段验证。
7. **技术路线已确定：**正式 Editor 使用 C++ 自研 UI；C# 用于计划中的游戏脚本 1.0，不用于 Editor UI。现有 ImGui 继续作为 Sandbox 调试工具。

参考截图对应的初始工作区：顶部菜单与工具栏、左侧 Content Browser、中间 Scene 视口、右上 Outliner、右下 Details。这只是可恢复的默认布局，不限制用户重新停靠或将面板拖到其他显示器。

## 逻辑架构

以下名称表示设计职责，**不是当前仓库中已经存在的类或接口**。

```mermaid
flowchart TB
    App["Limen Editor 独立应用"] --> Session["Editor 会话<br/>活动场景与统一选择状态"]
    App --> WindowHost["窗口宿主<br/>统一事件与帧调度"]

    WindowHost --> MainWindow["主窗口"]
    WindowHost --> FloatingWindows["浮动窗口 0..N"]
    MainWindow --> MainDock["停靠布局 / 标签组"]
    FloatingWindows --> FloatingDock["停靠布局 / 标签组"]

    Session --> Panels["面板实例<br/>Scene / Content / Outliner / Details"]
    Panels -. "停靠或移入" .-> MainDock
    Panels -. "拖出或移入" .-> FloatingDock
    Panels -->|"查询与编辑操作"| Session

    Session -->|"通过原生引擎管理"| Scene["C++ Scene / Asset"]
    Panels -->|"仅 Scene 视口发起渲染"| SceneRenderer["SceneRenderer / RHI"]
    SceneRenderer -->|"只读借用"| Scene
```

窗口、停靠位置和面板内容是三个不同层次：把 Outliner 从主窗口拖到浮动窗口，改变的是停靠关系；`Scene` 和选择状态仍属于同一个 Editor 会话。关闭浮动窗口后，场景继续存在。布局配置应与场景数据分别保存，避免移动面板触发场景文件修改；布局持久化的具体格式尚未设计。

## 当前代码与目标之间的差距

- `LimenEditor` 目前只有独立窗口和帧循环，没有 UI 面板、场景视口或编辑功能。
- 目前 [`Application`](../LimenEngine/include/Limen/Application/Application.h) 只持有一个 `Window`，主循环只轮询并呈现这一个窗口；多窗口宿主尚未实现。
- Sandbox 的 [`ImGUILayer`](../LimenEngine/src/Editor/ImGui/ImGUILayer.cpp) 启用停靠，但关闭了多原生窗口。仅开启 ImGui 的多视口标志不能替代正式 Editor 的窗口、面板和状态所有权设计。
- [`Example3DLayer`](../LimenSandBox/src/Example3DLayer.h) 将测试场景、`SceneRenderer` 与视口状态放在一个 Layer 中；正式 Editor 需要把跨面板共享的场景/选择状态与每个视口自己的状态分开。
- 当前 [`SceneRenderObjectHandle`](../LimenEngine/include/Limen/Scene/Scene.h) 只有数组下标。安全删除、跨面板选择和撤销前，需要按根目录 [README 的阶段 6](../README.md)完善对象身份及序列化；不把现有句柄直接当成稳定的 Editor 对象 ID。

## 尚待决定

- 多窗口下原生视口、输入焦点、高 DPI、GPU 呈现和图形 Context 的具体接入方式；不能默认逐帧把 GPU 图像读回 CPU。
- 面板实例创建/关闭规则、布局持久化格式，以及多个 Scene 视口的资源与相机生命周期。
- 多项目或多 Editor 会话是否在同一进程中运行；当前已确认的多窗口规则只针对同一个会话。

参考资料：[Epic：Slate Overview](https://dev.epicgames.com/documentation/unreal-engine/slate-overview-for-unreal-engine?lang=en-US) · [Epic：Layout Customization](https://dev.epicgames.com/documentation/en-us/unreal-engine/layout-customization?application_version=4.27)。借鉴的是可停靠面板、浮动窗口及布局管理的交互原则，不复制 UE 的实现。
