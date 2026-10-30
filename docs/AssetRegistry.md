# Asset Registry 与资产元数据（G1 设计与实施契约）

## 目标与当前状态

首个可验收结果是：在项目中注册一个现有资产，关闭并重新打开项目后，仍能用同一 `AssetID` 找到该资产；原文件丢失时能报告缺失引用，而不是悄悄生成新 ID。长期的导入设置、依赖追踪、重新导入和 Cook 以此为基础，但不要求第一版一次实现完毕。

当前 [`AssetID.h`](../LimenEngine/include/Limen/Asset/AssetID.h) 和 [`AssetID.cpp`](../LimenEngine/src/Asset/AssetID.cpp) 已提供 128 位 ID 的生成、小写 UUID 形态输出及严格解析，纯 CPU 测试位于 [`AssetIDTests.cpp`](../tests/AssetIDTests.cpp)。全零 ID 无效，解析失败不修改输出；格式化全零值仍会得到全零文本，因此“格式化后可解析”只对有效 ID 成立。[`AssetRegistry.h`](../LimenEngine/include/Limen/Asset/AssetRegistry.h) 与 [`AssetRegistry.cpp`](../LimenEngine/src/Asset/AssetRegistry.cpp) 已实现项目级 CPU-only 注册表和 `.lmmeta` 读写，但尚未接入 Editor 项目会话。项目的 `project_id` 与单个资产的 `AssetID` 是不同身份；当前项目配置加载器仍接受大写十六进制，以下小写规则只约束资产元数据中的 `asset_id`。

## 所有权和模块边界

- Registry 是**项目作用域、CPU-only 的实例**，输入为已校验的 `ProjectConfiguration::AssetRootAbsolute`；以后由活动项目会话独占。无项目的 Editor 外壳没有项目 Registry。关闭项目时释放 Registry，不依赖图形 Context。
- Registry 保存稳定身份与项目相对逻辑路径，不拥有 Texture、Model 或 GPU 句柄。现有 `AssetManager` 负责运行时 GPU 资源加载与缓存，不负责给资产分配持久 ID。
- Editor 与未来 Player 通过同一项目资产身份解析引用；发布版可由 Cooker 生成集中 manifest，不要求携带编辑器的 sidecar。
- `AssetPath::Resolve()` 只做词法路径拼接，且接受绝对输入，**不能**用它替代 Registry 的可移植路径与越界校验。

## 第一版磁盘格式

项目源资产使用同目录、同文件名再加 `.lmmeta` 的 sidecar。这样 `brick.png` 和 `brick.jpg` 各有独立元数据；团队并行导入时不必同时修改一份全项目清单。

```text
Game1/
├── Game1.lmproject
└── assets/
    └── Textures/
        ├── brick.png
        └── brick.png.lmmeta
```

```toml
# assets/Textures/brick.png.lmmeta，UTF-8（无 BOM）
schema_version = 1
asset_id = "d0360f3d-ef2e-41d3-a0b6-9bcc5940a539"
```

第一版只写这两个字段；使用项目已有的 TOML 解析依赖，不引入新解析库。`asset_id` 必须是非全零、36 字符、固定分组、十六进制字母全小写。逻辑路径由 sidecar 的位置去掉末尾 `.lmmeta` 后，相对资产根推导，不在 sidecar 中再存一份可与实际位置冲突的路径。资产内容不写入元数据；不要把本机绝对源路径或 GPU 句柄写入共享文件。将来加入 importer、设置、来源与依赖时，通过明确的 schema 演进和迁移处理，不静默接受或重写未知字段。

sidecar 属于项目源数据，应随资产纳入版本控制。没有 sidecar 的普通文件只是未注册资产：扫描不会替它生成 ID，也不会把临时路径当长期引用。把 sidecar 复制给另一资产会形成重复 ID，必须诊断而不能任选其一。sidecar 存在但原文件丢失时，保留记录和 ID，标记为缺失，供 Scene/Editor 报告断开的引用。

## 第一工作包：注册与重新打开（CPU 基线已实现）

1. 构造项目级 Registry，验证资产根，扫描 `.lmmeta`，解析版本与 ID，推导项目相对逻辑路径，建立 `AssetID → 记录` 和 `逻辑路径 → AssetID` 两个索引。扫描只读磁盘；错误不能留下半构建的索引。
2. 注册一个**已存在**、尚无 sidecar 的项目资产时，先验证路径和重复情况，再生成一次 ID、写出 sidecar，写入成功后才发布到内存索引并返回 ID。注册操作遇到已有 sidecar 应拒绝覆盖；加载/扫描流程负责读出原 ID，不能重新生成。第一版只考虑单进程写入。
3. 路径使用项目资产根下的 `/` 分隔相对路径，不接受绝对路径、盘符、反斜杠、空分量、`.`、`..`、跨平台保留名、大小写冲突或符号链接越界。磁盘文件的精确拼写必须与记录一致；不能靠 `lexically_normal()` 把危险输入变成看似有效的路径。
4. 无效 ID、错误 schema、重复 ID/路径和不安全路径应报告具体 sidecar/路径及原因。失败时不改调用方输出，也不发布部分索引；缺失原文件是可查询的 `Missing` 状态，不因此把旧 ID 抹掉。

首包的生产落点是 [`AssetRegistry.h`](../LimenEngine/include/Limen/Asset/AssetRegistry.h)（公共记录、错误及查询契约）和 [`AssetRegistry.cpp`](../LimenEngine/src/Asset/AssetRegistry.cpp)（磁盘读写、索引与内部哈希）。现有 `LimenEngine/CMakeLists.txt` 自动收录 `src/Asset/*.cpp`，不需要手动添加该实现文件。`AssetID` 的哈希与大小写冲突索引留在 Registry 内部，不泄漏到公共接口。

验收由 [`AssetRegistryTests.cpp`](../tests/AssetRegistryTests.cpp) 的纯 CPU 测试完成：临时项目里注册资产 → 销毁 Registry → 从不同工作目录重新打开 → ID 与相对路径不变；还覆盖重复 ID、格式错误、无 sidecar、缺失源文件、非法 UTF-8/路径、后缀大小写、失败时索引不变及成功注册后无暂存文件。不创建窗口或 GPU Context。2026-10-29 在 macOS 上以 `cmake --build --preset build-debug --target LimenAssetRegistryTests` 构建，并以 `ctest --test-dir out/cmake-build-debug-clang17 -R '^LimenAssetRegistry$' --output-on-failure` 验证通过。

当前首包仍有跨平台边界：路径大小写冲突检测只折叠 ASCII，不涵盖 Unicode 的大小写和规范化等价；较长的源文件名加上暂存目录后缀可能触及单个文件名长度上限；Windows 的设备名规则还需在目标平台上补充验证。不要把 macOS 的 CPU 测试通过写成 Windows 全面验收。

## 后续步骤，不计入首包完成

Editor 的项目会话接入 Registry 后，Content Browser 才能对真实资产做操作；移动或重命名时必须让原文件与 sidecar 成对移动并保持 ID，失败时要有回滚或明确的恢复诊断。随后补导入设置、外部来源记录、依赖关系与重新导入，再让场景和起始场景通过 `AssetID` 持久引用。不要把首包的“能重开同一个 ID”误报为这些能力已经完成。
