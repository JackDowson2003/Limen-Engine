# Limen Editor

> 状态：`LimenEditor` 已有单窗口 C++ 自研 UI：场景视口、Content Browser、Outliner、Details 基础面板，文字图集、矩形裁剪、单选/多选与批量删除已接入；真实 Retina 交互、资产驱动对象生成、停靠和多原生窗口仍待完成。C# 用于游戏脚本 1.0，不用于 Editor UI。当前迭代顺序：A（会话数据契约）→ B（真实场景视口）→ C（面板与文字）→ D（资产驱动生成），见下文。

## 当前已实现

- `EditorApp` 注册 `EditorUILayer`；引擎在场景更新、窗口清屏之后调用 `OnUIRender()` 绘制正式 Editor UI，不依赖 Sandbox 的 ImGui。
- `EditorUILayout` 使用逻辑窗口尺寸计算顶部、左侧、中央、右上、分隔条和右下六个区域；常规宽度下三栏保持比例，极窄窗口优先满足中央最小宽度，左右栏可能缩至零。
- `EditorUIRenderer` 通过 OpenGL 后端绘制纯色和纹理矩形，并区分逻辑窗口坐标与帧缓冲像素尺寸；目前用引擎白纹理进行最小绘制检查，文字、裁剪和输入焦点尚未接入。
- 右侧分隔条在鼠标按下后记录交互状态，松开时按右侧正文总高（包含分隔条）更新上下比例；不要求分隔条精确贴住松开位置，也不在移动时实时调整。零尺寸帧缓冲事件会取消拖动并清除旧命中区域。
- `LimenEditorUILayout` CPU 测试覆盖常规与放大窗口的布局比例、右侧总高、极矮窗口和矩形命中边界；它不替代窗口画面与输入验证。
- `LimenOpenGLEditorUITextureTests` 使用非对称 RGBA 纹理与隐藏 OpenGL 窗口读回像素，验证 UV、sRGB、Alpha 以及逻辑尺寸到 2 倍帧缓冲尺寸的映射；这不是实际 Retina 显示器或可见 Editor 窗口的验收。

## 已确认的架构规则

以下是正式 Editor 的目标规则；多窗口、面板停靠和共享编辑状态目前仍未实现。

1. **独立应用：**Limen Editor 与 `LimenSandBox` 是两个客户端，分别使用 C++ 引擎核心；正式功能不继续堆进 Sandbox，Sandbox 保留为渲染实验和回归入口。
2. **一个会话、多窗口：**同一个 Editor 会话可以管理主窗口和多个浮动原生窗口。窗口是面板的承载位置，不各自创建一份场景。一个统一的 UI/渲染调度者管理这些窗口；这不等于只能打开一个窗口。
3. **面板与窗口分离：**Scene 视口、Content Browser、Outliner 和 Details 是可停靠的面板。面板可以在主窗口内重排，也可以拖出成为浮动窗口，再重新停靠。默认布局参考 UE 的工作流，但不照搬其图标、样式或源码。
4. **共享编辑状态：**同一会话中的 Outliner、Details 和各窗口读取同一个活动场景与选择状态。拖动或关闭浮动窗口只影响窗口及其中的面板，不复制或销毁场景；重新打开面板后仍能访问该会话。
5. **区分数据所有权：**实际 `Scene`、Asset 和 GPU 资源由 C++ 引擎侧管理。Editor 会话管理当前场景的使用与编辑状态；窗口和普通面板只借用这些数据，不拥有或释放场景。`SceneRenderer` 渲染时只读取 `Scene`，不负责 Editor 窗口和停靠布局。
6. **视口有自己的视图状态：**Scene 视口面板可以拥有自己的相机、尺寸和显示状态，但引用同一活动场景。多个视口是否各自拥有 `SceneRenderer`、如何共享图形资源和处理 Context 生命周期，需在视口设计阶段验证。
7. **技术路线已确定：**正式 Editor 使用 C++ 自研 UI；C# 用于计划中的游戏脚本 1.0，不用于 Editor UI。现有 ImGui 继续作为 Sandbox 调试工具。
8. **会话独占活动场景：**`EditorSession` 以独占所有权持有活动 `Scene`；窗口与面板只借用 `const Scene&`，不拥有、不释放。一切修改经会话提供的方法完成，选择与脏状态不可能被面板绕过。对应 UE 的 GEditor 拥有编辑世界与选择集、面板不拥有 Actor。
9. **选择只存稳定身份：**选择状态以 `SceneObjectID` 为键，不保存运行期句柄或数组下标；对象销毁后旧 ID 自动失效并从选择中清除。接口先按单选实现，但形状兼容后续多选。
10. **脏状态显式转换：**成功的编辑操作置脏，保存（将来接入）清除脏；失败操作不改变脏状态。
11. **创建即选中：**新建对象自动成为当前选择，对应 UE 中 Spawn 后自动选中。

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

- 当前可见区域仍以单窗口占位矩形和白图为主；非对称纹理的 GPU 数值测试不等于正式面板已有真实数据，尚无 Scene 视口或编辑功能。
- 目前 [`Application`](../LimenEngine/include/Limen/Application/Application.h) 只持有一个 `Window`，主循环只轮询并呈现这一个窗口；多窗口宿主尚未实现。
- Sandbox 的 [`ImGUILayer`](../LimenEngine/src/Editor/ImGui/ImGUILayer.cpp) 启用停靠，但关闭了多原生窗口。仅开启 ImGui 的多视口标志不能替代正式 Editor 的窗口、面板和状态所有权设计。
- [`Example3DLayer`](../LimenSandBox/src/Example3DLayer.h) 将测试场景、`SceneRenderer` 与视口状态放在一个 Layer 中；正式 Editor 需要把跨面板共享的场景/选择状态与每个视口自己的状态分开。
- 当前 [`SceneObjectHandle`](../LimenEngine/include/Limen/Scene/Scene.h) 已带 Generation，稳定身份由 `SceneObjectID` 承担，渲染组件也已统一到对象槽；剩余差距是场景序列化、撤销/重做和跨面板编辑操作。

## 接下来执行的顺序

Editor 服务于根目录 [README 的 G0–G4 游戏交付主线](../README.md)，不是单独无限扩张的 UI 技术演示。每个节点都以真实项目数据和可重复操作验收：

1. **G0：收稳最小 UI 路径。**用非对称 RGBA 图验证纹理 UV、颜色、Alpha、缩放与高 DPI；保留已有“松开才调整、区域可接近收起”的分隔条交互，并复验窗口缩放、非全屏最大化、最小化恢复及非零尺寸变化到下一次绘制之间的旧命中区域。让 `LimenEditor` 有显式构建/测试入口。
2. **G1：等待并接入真实数据契约。**Engine 提供项目配置、稳定 AssetID、场景对象身份与保存/加载；Editor 会话借用活动 Scene/Asset，管理统一选择和脏状态。不能用临时数组下标充当跨面板长期引用。
3. **G2：做出可用的场景编辑器。**只补齐真实面板需要的文字、裁剪、焦点与事件路由；中央 Scene 视口消费 `SceneRenderer` 的最终颜色，按视口尺寸更新离屏目标与相机宽高比，并检查逻辑尺寸、帧缓冲像素尺寸及 GPU Context 生命周期。Hierarchy/Outliner、Inspector/Details、Content Browser 操作同一场景。加入 Picking、Transform 编辑、Undo/Redo、保存反馈；以“修改后关闭重开仍一致”验收。UI 不拥有或释放 Scene。
4. **G3–G4：接入游戏工作流。**Play/Stop 使用隔离的运行世界，Editor 将构建/发布入口连接到独立 Player、C# 游戏脚本和 Cooker；游戏能力及发布包由引擎主线负责，Editor 不复制一套运行时。
5. **后续工作区能力。**真实面板、输入焦点和资源生命周期稳定后，再做停靠、浮动原生窗口与独立于场景的布局持久化；多窗口共享同一个 Editor 会话，不是 G2 基本编辑闭环的前置条件。

上述是依赖与验收顺序，不表示目标能力已实现。OpenGL 实现留在 Editor 后端，不把图形 API 类型带入通用 UI 接口；新增字体等第三方依赖前仍需评估方案。C# 只用于 G3 起的游戏脚本。

## 当前迭代顺序：A → B → C → D

在 G0 收尾与 G1 会话接入之间按四个工作包推进，每包都有独立验收句；除另行约定外，生产代码由用户编写，Codex 负责设计与审查。

### A：会话数据契约（已完成）

- 范围：`src/Project/EditorSession.h` 扩展活动 Scene、统一选择与脏状态；`tests/EditorSessionTests.cpp` 覆盖选择、失效、多选、批量删除与脏标记。
- 关键接口：`CreateObject`（创建即选中）、`CreateRenderableObject`、`SelectObject`、`ToggleObjectSelection`、`SelectObjects`、`DeleteSelectedObjects`、`GetSelectedObjectIDs/Count`、`DeleteObject`、`SetSelectedObjectTransform`、`TryGetSelectedObjectTransform`、`GetScene`（const 借用）、`IsDirty/ClearDirty`。
- 验收：`LimenEditor` 编译通过；`LimenEditorSession` 测试全绿。

### B：真实场景视口（已完成）

- 中央区域升级为真视口：视口组件拥有自己的相机与 `SceneRenderer`，共享会话的活动 Scene；`OnUpdate` 渲染离屏、`OnUIRender` 把最终颜色画进中央矩形。
- 新增：UI 后端“原生纹理句柄”绘制入口、直通采样、hover 焦点路由，以及 UE 式右键环视/飞行、中键平移、滚轮推拉缩放。
- 验收：启动 `LimenEditor --project <项目>`，中央视口显示场景，视口内可旋转/缩放相机，窗口放大两倍时清晰不变形。

### C：面板骨架与文字（已完成基础）

- 补齐字体图集、文字渲染与矩形裁剪；Content Browser 列出资产路径，Outliner 列出场景对象并支持 Create、单选、Command/Ctrl 多选、Shift 范围选择；Details 显示单选 Transform 或多选数量。
- 引擎侧新增 `Scene::ForEachObject/ForEachRenderableObject` 与 `AssetRegistry::ForEachRecord`，面板只读借用数据。
- 验收：面板显示会话真实数据；选择、删除在面板间同步；字体方向、裁剪和纹理直通 GPU 测试通过。

### D：资产驱动对象生成（下一步）

当前 Create 生成临时立方体，只用于打通渲染链路。D 将其替换为真实项目资产工作流，参考 UE 的 Content Browser 选中资产 → Spawn/Place 流程：

1. Content Browser 行点击调用 `EditorSession::SelectAsset(AssetID)`，选中行高亮；资产选择与场景对象选择互相独立。
2. Instantiate/Create 要求存在选中资产且 `AssetRecordStatus::Present`；资产缺失或加载失败时不修改 Scene，并给出可见错误。
3. 通过 `AssetRegistry` 的 `LogicalPath` 调用 `AssetManager::LoadModel` 加载模型；用 `ModelMaterialBuilder::BuildBlinnPhong` 和 `EditorSceneContent` 的 Blinn-Phong Pipeline 构建运行时材质。
4. 第一版只支持模型资产（OBJ + sidecar）；纹理只作为材质输入，不作为可生成对象。一个多 part 模型暂时生成多个 SceneObject（part 级选择），真正“一个资产实例 = 一个层级对象”需要 Scene 层级/组件模型后再做。
5. 每个 part 的 Transform 使用 `ModelPart::LocalTransform`；同一模型实例的多个 part 共享 Mesh/Material 的 `Ref`。生成后对象加入选择，Outliner/Details 立即刷新。
6. GPU 资源由 Editor 渲染层与 AssetManager 持有，Scene 只借用；临时立方体保留为开发调试路径，正式生成流程不依赖它。

文件范围：`EditorSession`（资产选择已就绪）、`EditorSceneContent`（暴露 Blinn-Phong Pipeline）、新增 Editor 侧的 Model Spawner、`EditorUILayer`（资产行 ID、高亮、Create 行为）、测试与资产样例。验收：选中一个已注册模型资产 → Create → 视口出现模型，Outliner 出现对应 part，Details 显示 Transform；重复实例共享 Mesh/Material；缺失资产不留下半成品对象。

### UE5 参照边界

借鉴 GEditor 全局选择集、Package Dirty 的脏标记、ViewportClient 每视口独立视图状态、Content Browser 资产选择 → Spawn、Spawn 即选中；不照搬 Slate 停靠框架、反射式 Details 面板与多窗口宿主（G2 之后再议）。

## 尚待决定

- 多窗口下原生视口、输入焦点、高 DPI、GPU 呈现和图形 Context 的具体接入方式；不能默认逐帧把 GPU 图像读回 CPU。
- 面板实例创建/关闭规则、布局持久化格式，以及多个 Scene 视口的资源与相机生命周期。
- 多项目或多 Editor 会话是否在同一进程中运行；当前已确认的多窗口规则只针对同一个会话。

参考资料：[Epic：Slate Overview](https://dev.epicgames.com/documentation/unreal-engine/slate-overview-for-unreal-engine?lang=en-US) · [Epic：Layout Customization](https://dev.epicgames.com/documentation/en-us/unreal-engine/layout-customization?application_version=4.27)。借鉴的是可停靠面板、浮动窗口及布局管理的交互原则，不复制 UE 的实现。
