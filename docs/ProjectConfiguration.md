# 项目配置与跨平台启动（G1 设计与实施契约）

本文件规定 Limen 项目如何被识别、打开和交给 Editor/Player。当前 `TryLoadProjectConfiguration` 已能独立校验 `.lmproject` 后缀、TOML 核心字段和资源根路径；Editor 的 `--project` 已在创建 `Application` 前读取配置，并把项目资产根传给 `AssetManager`。引擎内置 Shader 由可执行文件位置独立定位，不再随项目资产根或当前工作目录改变；Player 接线、运行设置合并、配置保存及起始场景仍未实现。首个完整验收目标仍是同一项目从不同工作目录打开后，真实项目资产与起始场景均保持一致。

## 文件命名与职责

Limen 项目描述文件使用 `.lmproject` 后缀，例如 `Game1.lmproject`。它是**单个文件**，内容采用 TOML，不是一个目录，也不是另造一种解析语法。项目级运行设置继续使用普通 `.toml`，不因为属于 Limen 就为每个配置文件发明新扩展名。未来场景、资产格式如需自有扩展名，应在各自格式/schema 确定时单独命名。

此前考虑的 `.lproj` 已由 Apple 用作 macOS 本地化资源**目录**；为避免与现有工具链约定混淆，项目文件改用 `.lmproject`。[Apple 对 `.lproj` 目录的说明](https://developer.apple.com/documentation/xcode/localizing-package-resources)。当前加载器已要求后缀精确等于 `.lmproject`；其他后缀返回 `InvalidPath`，并且不修改调用方原有的配置输出。

建议的源码项目布局（以下文件均为**规划**，不是当前仓库已有的示例项目）：

```text
Game1/
├── Game1.lmproject              # 共享的项目身份与资源入口；TOML 内容
├── assets/                      # 项目资产，名称大小写固定
│   └── Scenes/
├── Config/
│   ├── DefaultGame.toml         # 共享的游戏运行默认值
│   └── Platforms/
│       ├── macOS.toml           # 可选：macOS 的运行设置覆盖
│       └── Windows.toml         # 可选：未来 Windows 后端完成后启用
└── Saved/
    └── Config/
        ├── UserGame.toml        # 可选：本机可覆盖的运行偏好
        └── EditorUser.toml      # 本机 Editor 布局、最近打开项等
```

| 文件/数据                      | 归属与持久化规则                                                                                                                                                                                        |
|--------------------------------|---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------|
| `Game1.lmproject`              | 项目创建时生成，纳入版本控制；保存稳定项目身份、显示名、相对资源根目录和配置 schema 版本，不保存用户窗口位置或 GPU 句柄。                                                                               |
| `Config/DefaultGame.toml`      | 创建项目时生成，纳入版本控制；保存共享的起始场景与运行默认值，Editor 预览与 Player 默认读取同一来源。空项目可以暂不指定起始场景。                                                                       |
| `Config/Platforms/*.toml`      | 只覆盖白名单内的平台运行设置，不改项目身份、资产根目录、AssetID 或场景文件；操作系统与图形后端是两个独立维度。                                                                                          |
| `Saved/Config/UserGame.toml`   | 源码项目工作副本中可选的本机用户偏好，仅覆盖明确允许的显示/音量等运行设置；不改项目身份或起始场景。建立该目录时加入项目忽略规则。发布版应把同类用户设置放到操作系统的可写用户数据目录，不能写回应用包。 |
| `Saved/Config/EditorUser.toml` | 仅归 Editor 本机会话所有，不参与 Player 配置合并；不把布局、选择状态写进共享场景或项目描述。                                                                                                            |
| 构建配置、Cook manifest        | CMake Preset 负责构建工具链；Cook manifest 是构建产物。二者都不是游戏运行配置，也不混进 `.lmproject`。                                                                                                  |

`.lmproject` 和 `.toml` 均按 UTF-8（无 BOM）保存。当前项目描述文件由 `toml11` 按 [TOML 1.0.0](https://toml.io/en/v1.0.0) 模式解析，并转换为 Limen 自有的 `ProjectConfiguration`；第三方类型不泄漏到 Engine/Editor 公共接口。运行设置文件的读取、合并和写出尚未实现，不能因项目描述文件已可读取就认为这些配置能力也已完成。

## 第一版字段边界

下方 `.lmproject` 示例是**当前加载器已实现的核心 schema**：`schema_version = 1`，读取 `project_id`、可选的 `display_name` 与 `asset_root`。后面的 `DefaultGame.toml` 示例仍是运行设置的**拟定 schema**，尚无加载器。`project_id` 是创建项目后不随显示名或目录重命名改变的身份；它与后续单个资源的 AssetID 不同。

```toml
# Game1.lmproject：扩展名为 .lmproject，内容仍是 TOML
schema_version = 1
project_id = "d0360f3d-ef2e-41d3-a0b6-9bcc5940a539"
display_name = "Game1"
asset_root = "assets"
```

```toml
# Config/DefaultGame.toml
schema_version = 1

[game]
startup_scene = "Scenes/Main.lscene" # 仅示意逻辑路径；场景格式尚未定案

[render]
backend = "Auto"
vsync = true
```

- `.lmproject` 的必填字段是 `schema_version`、`project_id` 和 `asset_root`；`display_name` 可默认取文件名。`asset_root` 解析为项目文件所在目录下的路径，而不是进程工作目录下的路径。
- `[game].startup_scene` 在可运行游戏项目中必须解析为项目资产；项目刚创建、尚无场景时可以省略。TOML 没有用来表示“未设置”的 `null` 字段，此处不用空字符串伪装有效场景。将来 Asset Registry 成立后，长期引用应改由稳定 AssetID 表示，不能把可重命名路径当永久身份。
- `[render].backend = "Auto"` 表示从**已编译且当前平台实际可用**的后端中选择，不是从 `RendererAPI::API` 的所有枚举值中任选。明确指定不可用后端时，应在创建 Window/Graphics Context 前报错并停止启动，不静默降级。
- 第一版仅承诺 macOS + OpenGL 4.1 可运行。Windows + Direct3D 12、macOS + Metal 等是未来目标；配置格式可表达目标，不代表后端已经实现。OS 覆盖文件也不能为不存在的后端“开通支持”。
- 身份、资源根和起始场景不接受 `Platforms`、`UserGame` 覆盖；命令行如要临时指定测试场景，必须是显式的启动参数且不回写项目文件。

## 合并、路径与启动顺序

运行设置按“引擎内建默认值 → `DefaultGame.toml` → 当前**目标操作系统**的 `Platforms/<OS>.toml` → 可选的 `UserGame.toml` → 显式命令行参数”合并。每层只覆盖允许的运行字段；缺失的可选文件不报错，格式错误和不允许的字段必须给出文件名与键路径。源码项目的 `UserGame.toml` 位于 `Saved/Config/`；发布版从以稳定 `project_id` 隔离的系统用户数据目录读取/写入同类设置，绝不写到只读发布包内。`EditorUser.toml` 使用独立加载链，不参与上述优先级。目标 OS 由启动目标/运行环境明确决定，不能仅凭一个后端名猜测。

项目自有路径使用 `/` 作为配置中的逻辑分隔符，相对于 `.lmproject` 所在目录或明确的 `asset_root` 解析；不把宿主机绝对路径、盘符、`..` 越界路径写进共享项目文件。解析时还需检查符号链接逃逸与大小写冲突，避免 macOS/Windows 上看似可用、换机器后指向别的文件。外部源文件的导入与重导入记录属于后续 Asset Registry，不能通过放宽项目路径规则偷渡。引擎内置 Shader/Editor 资源与项目 `assets/` 分属不同资源域，均不得靠当前工作目录偶然找到。

### `asset_root` 的跨平台路径规则

`.lmproject` 是共享项目文件，因此 `asset_root` 采用**统一的可移植规则**，不根据当前运行的是 macOS 还是 Windows 放宽校验。这样 `assets/CON` 即使在 macOS 上能创建，也会在加载配置时报告无效路径，而不是等项目迁移到 Windows 后才失败；普通目录名（例如 `assets`）不会因此报错。当前规则只约束项目资源根；未来资产树中每个资源路径的校验和重命名冲突检测由 Asset Registry 承接。

1. TOML 值是 UTF-8 字符串，以 `/` 分隔，必须是相对于 `.lmproject` 所在目录的路径；拒绝绝对路径、盘符、反斜杠、空分量、`.`、`..`、控制字符和跨平台保留字符，不通过自动规范化来掩盖错误。
2. 对每个目录分量拒绝末尾空格或句点，以及 Windows 保留设备名：`CON`、`PRN`、`AUX`、`NUL`、`COM1`–`COM9`、`LPT1`–`LPT9`、`COM¹/²/³`、`LPT¹/²/³`。比较时忽略这些 ASCII 名称的大小写；带扩展名仍无效，例如 `CON.txt`。这不是 Windows 后端的特殊开关，而是项目格式本身的约束。[微软的文件和目录命名规则](https://learn.microsoft.com/en-us/windows/win32/fileio/naming-a-file)
3. 路径必须指向已存在的目录。逐级核对配置中的分量与实际目录项的**准确拼写**；若磁盘上是 `assets`，配置写作 `ASSETS`，即使当前文件系统允许访问，也应拒绝，避免在区分大小写的文件系统上失效。此处只验证引用路径，不扫描整个资产树寻找所有大小写冲突。
4. 解析符号链接后，按路径分量验证最终目录仍在规范化的项目根目录内；不能用字符串前缀判断包含关系。任一校验失败返回 `InvalidPath`，错误键为 `asset_root`，不修改调用方原有的 `ProjectConfiguration`，也不静默改写 `.lmproject`。

纯 CPU 回归测试位于 [`tests/ProjectConfigurationTests.cpp`](../tests/ProjectConfigurationTests.cpp)：覆盖后缀、正常路径、保留设备名、大小写拼写不符、符号链接越界和失败时输出不变。后缀错误属于整个项目文件路径错误，`ProjectConfigurationError.Key` 为空；`asset_root` 错误的键仍为 `asset_root`。当前测试若仍有失败，表示实现尚未满足上述契约，而不是允许平台特例。

完整的直接打开项目与未来 Player 启动流程应在 `Application`、Window 和图形 Context 创建**之前**完成：启动入口取得 `.lmproject` 路径 → 校验并解析配置 → 按目标平台合并运行设置 → 验证资源与后端能力 → 构造活动项目会话 → 再初始化 Asset、Window 与 Renderer。当前 Editor `--project` 只完成核心配置预检与项目资产根传递，暂沿用固定的 OpenGL 启动设置；它不等于项目会话、运行设置合并或后端选择已完成。Editor 若先显示“选择/创建项目”窗口，可以先建立**无项目资源**的 Editor 外壳，再按同一规则打开项目；项目资产、场景和依赖项目配置的 Renderer 状态必须等校验通过后初始化。如果切换项目需要更换图形后端，第一版应明确要求重启或安全重建 Context，不能在旧 Context 上直接改枚举值。

项目会话应拥有解析后的完整配置值和已规范化路径；Asset、Scene、Renderer 只使用各自需要的结果，不持有 TOML 解析器对象。关闭项目或应用时按现有 GPU Context 生命周期释放资源。目前 [EntryPoint.h](../LimenEngine/include/Limen/EntryPoint.h) 已把命令行传给客户端，[EditorApp.cpp](../Limen-Editor/src/EditorApp.cpp) 已将绝对项目资产根交给 [Application.cpp](../LimenEngine/src/Application/Application.cpp)，[EditorSession.h](../Limen-Editor/src/Project/EditorSession.h) 已持有完整项目配置；项目级 Asset Registry 尚未接入会话。资产身份与元数据的下一步契约见[Asset Registry 与资产元数据设计](AssetRegistry.md)。

对于不支持的 `schema_version`、无效 `project_id`、重复或未知核心键、越界/缺失资源、不可用后端，应拒绝启动或拒绝保存并报告具体文件及键；不要用默认值掩盖拼写错误。旧版本迁移必须显式、可备份、可回滚，不能读取时顺手重写共享文件。保存配置应采用临时文件加原子替换，失败时保留原文件。

## 实施与验收顺序

1. **G1 配置契约与纯 CPU 测试**：已确定 `.lmproject` 后缀，项目描述加载器使用 `toml11` 的 TOML 1.0.0 模式，校验核心字段、路径及错误时输出不变；相关纯 CPU 测试已建立。项目配置写出/往返、可选运行配置、覆盖白名单与平台合并仍待实现，不将它们计入当前加载器的完成范围；不修改 CLion 维护的 `.gitmodules`。
2. **G1 启动接线**：Editor 已在构造 `Application` 前加载核心项目配置并传入绝对项目资产根；内置 Shader 与项目资产分别定位。`ProjectConfigurationTests` 与 `PlatformPathsTests` 各自验证跨工作目录路径，Renderer2D GPU 测试也已从仓库外工作目录通过；临时项目从仓库外启动 Editor 到达 OpenGL 4.1 初始化。仍需让 Editor 实际消费项目资产，并从仓库根、`out/` 和其他工作目录重复打开同一项目，才算完整验收。未来 Player 复用相同契约。
3. **G1–G2 场景/Editor**：项目创建、打开、关闭与重开使用同一 `.lmproject`；保存场景并做往返测试，Editor 本机布局与场景独立。此时再锁定场景格式与 AssetID 引用。
4. **G3–G4 Player/发布**：独立 Player 读取同一项目起始场景；不支持后端给出明确错误；Cook 后使用发布包内资源，脱离源码与 Editor 工作目录冷启动。平台覆盖要在对应后端真实可用后分别验证。

本文档本身不负责构建接线。当前加载接口位于 [`ProjectConfiguration.h`](../LimenEngine/include/Limen/Project/ProjectConfiguration.h)，实现位于 [`ProjectConfiguration.cpp`](../LimenEngine/src/Project/ProjectConfiguration.cpp)；后续 Editor 会话、Player 接线与配置保存仍须在各自模块中明确所有权、失败语义和构建目标。
