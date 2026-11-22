# Scene 对象模型与序列化（G1 设计与实施契约）

## 1. 状态与目标

本文定义 Scene 的持久对象模型、`.lscene` 文件格式、保存/加载语义以及 Engine 与 Editor 的职责边界。

当前已经具备：

- `SceneObjectID` 稳定对象身份；
- `SceneObjectHandle` 运行期 `Index + Generation` 访问句柄；
- 对象槽位复用、旧句柄失效和 Generation 退休；
- 一个 SceneObject 挂多个 `SceneRenderObject`；
- `AssetID` 与项目级 `AssetRegistry`；
- Editor 的资产放置、选择、Transform 查看、多选和脏状态。

当前尚未具备：

- 可序列化的对象名称；
- 与 `glm::mat4` 分离的 Authoring 级 TRS Transform；
- 稳定 ComponentID；
- 对象来源资产或组件来源数据；
- `.lscene` schema、保存器、加载器和原子替换；
- Project 起始场景；
- Scene 往返加载测试。

本文描述的是目标契约。除“当前已经具备”外，不应把本文内容误报为已实现功能。

## 2. 当前实现阻塞点

- `SceneRenderObject` 只有运行期 `Ref<Mesh>`、`Ref<Material>` 和局部矩阵，没有可持久化的资源身份，见 [`Scene.h`](../LimenEngine/include/Limen/Scene/Scene.h)。
- Scene 槽位只有 `SceneObjectID`、对象矩阵和运行期渲染组件，没有 Label、Authoring Component 或来源资产。
- [`EditorAssetSpawner.cpp`](../Limen-Editor/src/Render/EditorAssetSpawner.cpp) 已取得模型 `AssetID`，但完成运行期实例化后丢弃来源信息。
- [`EditorSession.h`](../Limen-Editor/src/Project/EditorSession.h) 有脏状态，但没有场景路径、保存命令或成功加载后的 Scene 替换操作。
- [`ProjectConfiguration.h`](../LimenEngine/include/Limen/Project/ProjectConfiguration.h) 尚无起始场景字段。

不能通过“Editor 侧临时保存 SceneObjectID → AssetID 的旁表”作为正式方案。Player、Undo、资源重命名和重新导入会需要另一套逻辑，最终形成两个编辑世界。

## 3. UE 参照与范围

借鉴 UE 的职责划分，不复制 `UObject`、反射、Slate、Gameplay Framework 或完整 Actor/Component 生命周期。

| UE | Limen v1 | 持久化 |
|---|---|---|
| `UWorld/ULevel` | `Scene` / `SceneDocument` | 是 |
| `AActor` | `SceneObject` | 是 |
| Actor GUID | `SceneObjectID` | 是 |
| `AActor::GetActorLabel` 一类显示名 | `SceneObjectLabel` | 是 |
| `UActorComponent` | `SceneObjectComponent` | 是 |
| Component GUID | `SceneComponentID` | 是 |
| `FTransform` | `SceneTransform` | 是 |
| `UStaticMeshComponent` 的来源资产 | `ModelRendererComponent` + `AssetID` | 是 |
| Soft Object Path | `AssetID` | 是 |
| Runtime Actor/Component 指针 | `SceneObjectHandle`、`Ref<Mesh/Material>` | 否 |
| 选择和 Gizmo 状态 | EditorSession | 否 |
| 视口相机、停靠布局 | `EditorUser.toml` | 不属于 Scene |
| Undo Transaction | Editor 命令历史 | 否 |

第一版不引入：

- 完整 UObject/反射系统；
- 任意脚本组件和运行时序列化；
- 大型 ECS/Job System；
- 父子 Attachment 层级；
- Blueprint/嵌套 Prefab；
- 多 Scene Streaming。

第一版对象的 Transform 是 Scene 世界空间 Transform。父子层级以后通过明确的 schema 版本演进加入。

## 4. 身份、名称与所有权

### 4.1 身份

- `SceneObjectID` 是对象持久身份，重新加载后保持不变。
- `SceneComponentID` 是对象组件持久身份，重新加载后保持不变。
- `SceneObjectHandle` 只用于当前 `Scene` 生命周期，不序列化。
- `AssetID` 是资源引用身份；Scene 不保存绝对路径或数组下标作为长期引用。
- 所有 ID 使用小写、固定分组、带 variant/version 位的 UUID 文本。

### 4.2 名称

- Label 是用户可见名称，可编辑、允许重复。
- Label 不承担身份，不替代 `SceneObjectID`。
- Editor 放置模型时默认从资产文件名生成 Label，例如 `TestCube`；冲突时生成 `TestCube_2`。
- Outliner 显示 Label；Label 缺失或非法时回退显示稳定 ID 的短后缀。

### 4.3 所有权

- `Scene` 拥有对象和组件的 Authoring 数据。
- SceneSerializer 只读取和生成 CPU 数据快照，不发出 GPU 命令。
- `AssetManager` 拥有解析后的 Model、Mesh、Material 与 GPU 缓存。
- `SceneRenderer` 只消费解析后的运行期渲染项。
- EditorSession 拥有活动 `Scene`；面板只借用并经过会话命令修改。

## 5. Authoring 数据与运行期数据

Authoring 数据描述“保存什么”；运行期数据描述“现在如何渲染”。两者不能混为一类。

### 5.1 SceneTransform

Authoring Transform 使用 TRS：

```cpp
struct SceneTransform
{
    glm::vec3 Location{0.0f};
    glm::quat Rotation{1.0f, 0.0f, 0.0f, 0.0f};
    glm::vec3 Scale{1.0f};

    glm::mat4 ToMatrix() const;
};
```

- Quaternion 固定顺序为 `x, y, z, w`。
- 序列化保存 Location、Rotation、Scale，不保存运行期矩阵。
- 所有分量必须是有限浮点数。
- 使用 Engine 世界单位，序列化层不做隐式单位换算。

### 5.2 SceneObjectAuthoringData

```cpp
struct SceneObjectAuthoringData
{
    SceneObjectID ID;
    LString Label;
    SceneTransform Transform;
    LArray<SceneObjectComponent> Components;
};
```

### 5.3 第一版组件

```text
ModelRendererComponent
    SceneComponentID
    SceneTransform LocalTransform
    AssetID ModelAssetID

PrimitiveRendererComponent
    SceneComponentID
    SceneTransform LocalTransform
    PrimitiveType = Cube

PointLightComponent
    SceneComponentID
    SceneTransform LocalTransform
    Color
    Intensity
```

- 一个 OBJ 模型是“一个 SceneObject + 一个 ModelRendererComponent”。
- Model 的 Front、Back、Left 等 part 是加载 Model 后得到的运行期子项，不序列化成多个对象。
- 第一版不序列化 Mesh/Material 的 `Ref`。
- 第一版没有 Material Override；材质由 Model 导入结果或 Primitive 默认材质解析。
- Component 数组下标不是 ComponentID。

### 5.4 运行期解析结果

加载或编辑 ModelRendererComponent 后，Engine 通过 `AssetManager` 得到 Model，再把各 ModelPart 展开为 `SceneRenderObject` 一类运行期渲染项。

`SceneRenderer` 不应在每帧通过 AssetID 重新加载模型。对象或组件变化时重建并缓存运行期渲染项。

## 6. `.lscene` 文件格式

### 6.1 文件与资产身份

- 场景文件使用 UTF-8（无 BOM）TOML，扩展名为 `.lscene`。
- `.lscene` 是项目资产，使用同目录 `.lscene.lmmeta` 保存其 `AssetID`。
- Project 的起始场景使用 `startup_scene_asset_id`，不把可重命名逻辑路径作为长期身份。
- `.lscene` 文件内容不重复保存 Scene AssetID；Scene 资产身份来自 sidecar。

目录示例：

```text
assets/Scenes/
├── Main.lscene
└── Main.lscene.lmmeta
```

### 6.2 第一版 schema

```toml
schema_version = 1
display_name = "Main"

[environment]
ambient_color = [0.1, 0.1, 0.1]
ambient_intensity = 1.0
directional_direction = [-0.5, -1.0, -0.3]
directional_color = [1.0, 1.0, 1.0]
directional_intensity = 1.0

[[objects]]
id = "aaaaaaaa-bbbb-4ccc-8ddd-eeeeeeeeeeee"
label = "TestCube"
location = [0.0, 0.0, 0.0]
rotation = [0.0, 0.0, 0.0, 1.0]
scale = [1.0, 1.0, 1.0]

[[objects.components]]
id = "11111111-2222-4333-8444-555555555555"
type = "model_renderer"
model_asset_id = "00112233-4455-6677-8899-aabbccddeeff"
location = [0.0, 0.0, 0.0]
rotation = [0.0, 0.0, 0.0, 1.0]
scale = [1.0, 1.0, 1.0]

[[objects]]
id = "bbbbbbbb-cccc-4ddd-8eee-ffffffffffff"
label = "KeyLight"
location = [1.0, 2.0, 0.0]
rotation = [0.0, 0.0, 0.0, 1.0]
scale = [1.0, 1.0, 1.0]

[[objects.components]]
id = "22222222-3333-4444-8555-666666666666"
type = "point_light"
color = [1.0, 0.8, 0.6]
intensity = 4.0
```

### 6.3 Schema 规则

- 第一版采用严格 schema：未知的必选语义字段、错误类型和重复 ID 必须报错。
- 数值数组长度必须精确匹配。
- Rotation 先要求非零，再归一化；不允许 NaN/Inf。
- ID 必须能严格解析且不能重复。
- `objects.components` 缺失表示不可渲染对象；是否允许空对象由 Scene 契约决定。
- 第一版不允许 `parent_id` 或复杂引用表达式。
- 保存顺序稳定，便于 diff：objects 按 Scene 稳定枚举顺序写出，components 按对象内顺序写出。
- 不保存绝对路径、GPU Handle、运行期 Generation、选择或相机状态。

## 7. 保存语义

```text
活动 Scene
    ↓
SceneDocument CPU 快照
    ↓
严格校验
    ↓
同目录临时文件
    ↓
写入、flush、close
    ↓
原子替换目标 .lscene
    ↓
EditorSession::ClearDirty
```

- 临时文件必须与目标文件位于同一目录，保证替换可在常见文件系统中原子完成。
- 写出或校验失败时保留旧文件，不清除脏状态。
- 第一版不自动保存。
- `.lmmeta` 不由每次场景保存重写；场景身份应保持稳定。
- 保存失败返回文件名、失败阶段和解析/写入诊断。

## 8. 加载语义

```text
读取 .lscene
    ↓
解析到 SceneDocument
    ↓
验证 schema、ID、Transform、组件
    ↓
通过 AssetRegistry/AssetManager 解析 AssetID
    ↓
构建临时 Scene
    ↓
全部成功
    ↓
替换 EditorSession 活动 Scene
```

- 加载过程中不能直接修改当前活动 Scene。
- 任一资产缺失或模型加载失败时，第一版拒绝整个加载，当前 Scene 保持不变。
- 后续版本再增加 Missing Asset 占位对象，但必须显式标记，不能静默生成新 AssetID。
- 成功替换后清空旧选择、清除脏状态并更新场景路径。
- 失败时保留原场景、原选择、原脏状态和原场景路径。
- 加载日志必须包含对象 ID、Label、ComponentID、AssetID 和具体错误。

## 9. 错误模型

建议使用结构化错误，而不是只返回一个字符串：

```cpp
enum class SceneFileErrorCode
{
    None,
    FileAccess,
    InvalidToml,
    UnsupportedSchema,
    InvalidField,
    InvalidID,
    DuplicateID,
    MissingAsset,
    AssetLoadFailed,
    InstantiateFailed,
    WriteFailed,
    AtomicReplaceFailed
};
```

错误至少包含：

```text
文件路径
schema_version
TOML 键路径
SceneObjectID
Label
SceneComponentID
AssetID
可读错误原因
```

## 10. Engine 与 Editor 边界

### 10.1 Engine 改动

以下属于 Engine 数据契约，应先由用户实现：

1. `SceneObjectIDToString/TryParseSceneObjectID`。
2. `SceneComponentID`、格式化与严格解析。
3. `SceneTransform` 与 `ToMatrix()`。
4. SceneObject Label 的 Get/Set/Restore。
5. Authoring Component 数据与稳定 ComponentID。
6. `Scene::RestoreObject(const SceneObjectAuthoringData&)`：保留 ObjectID、分配新 Handle/Generation、拒绝重复 ID。
7. 只读 Authoring 快照遍历，供保存器使用。
8. `SceneDocument`、`SceneSerializer`、严格 schema 与结构化错误。
9. ProjectConfiguration/运行设置增加 `startup_scene_asset_id`。
10. CPU 序列化测试和 Scene 实例化测试。

### 10.2 Editor 改动

Engine 契约稳定后由 Editor 实现：

1. New/Open/Save/Save As 命令。
2. `Cmd+S`、保存失败状态和窗口标题脏标记。
3. 成功加载后才替换活动 Scene。
4. Outliner 显示和编辑 Label。
5. Details 编辑 TRS 与组件字段。
6. 加载后恢复场景对象，不恢复 Editor 选择或相机。
7. Editor 关闭前未保存确认。
8. Project `StartupSceneAssetID` 对应的启动加载。

## 11. 实施顺序与验收

### A. Engine Authoring 数据

- SceneObject 增加 Label、TRS、Authoring Component；
- 保留现有运行期渲染缓存；
- `SceneObjectID` 在保存/恢复后不变；
- 旧 Handle 与 Generation 测试继续通过。

验收：创建 → 设置 Label/TRS/组件 → 快照读取 → 值一致；删除后旧 Handle 失效。

### B. SceneSerializer CPU 基线

- 写 `.lscene`、重新解析、值完全一致；
- 无 Window、GPU Context 和 AssetManager GPU 资源；
- 严格覆盖 schema、重复 ID、非法 Transform、未知类型。

验收：同一 `SceneDocument` 往返后对象数、ID、Label、Transform、ComponentID、来源 AssetID 与灯光参数一致。

### C. Asset 解析与临时 Scene 加载

- 使用 Assets/AssetManager 解析 ModelRenderer；
- 一个 Model 展开多个运行期 part，但 Outliner 仍只有一个对象；
- 缺失资产、模型失败和 Primitive 失败返回结构化错误。

验收：有效场景加载成功；任一坏引用时当前 Scene 完全不改变。

### D. Editor 保存/加载闭环

- 放置两个模型、修改 Label/Transform、点击 Save；
- 关闭进程并重新打开项目；
- 自动加载 `startup_scene_asset_id`；
- 对象数、身份、Label、Transform、组件和资源引用一致；
- 选择、相机和面板布局不被写入 Scene。

这是 G1/G2 场景保存闭环的验收条件。

## 12. 已锁定的设计决定

1. 使用 Actor-lite 对象与 Component 数据，不使用 Scene 数组下标作为身份。
2. 使用稳定 ObjectID 与 ComponentID。
3. Label 是可见名称，不是身份。
4. Authoring Transform 使用 TRS。
5. 资源引用使用 AssetID。
6. Model part 是运行期展开结果，不是独立持久对象。
7. `.lscene` 第一版使用严格 TOML schema。
8. 保存采用临时文件与原子替换。
9. 加载先构建临时 Scene，成功后再替换活动场景。
10. 第一版缺失资产导致加载失败，不静默修复引用。
11. Editor 选择、相机和布局不写入共享 Scene。
12. 父子层级、Material Override、Prefab 和 Missing Asset 占位留到后续 schema。
