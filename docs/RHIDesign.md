# RHI 跨后端接口设计（设计契约，尚未实现）

> 状态说明：本文描述目标接口与迁移顺序，**不代表这些类已经存在**。当前实现仍是 `RendererAPI`/`RendererCommand` + macOS OpenGL 4.1 后端；D3D12 后端尚未实现。

## 1. 目标与范围

Limen 的 RHI 目标只有一条：**上层 Renderer、SceneRenderer、Material、Editor UI 只依赖 Limen 自己的统一接口，OpenGL 与 Direct3D 12 的差异全部收在后端内部。**

本设计的直接动机是 OpenGL → D3D12 的可移植性。当前代码里已经出现了只在 OpenGL 下成立、换后端必错的约定：

- `VertexArray` 直接表达 GL 的 VAO；D3D12 的输入布局属于 PSO。
- Shader 用 `SetFloat("名字")` 逐项设置 uniform；D3D12 需要常量缓冲和显式绑定。
- `SetScissor/SetViewport` 使用左下原点像素坐标；D3D12/Metal/Vulkan 使用左上原点。
- `Texture2D::SetData` 没有明确行序契约；UI 和字体各自用 UV 翻转互相补偿，换后端会翻。
- Editor 视口通过 `uintptr_t` 接收原生纹理句柄，并调用 GL 专用绘制路径。
- 资源接口暴露 `Bind/UnBind`，把 GL 的状态机思维扩散到了公共层。

本文先固定契约，再按迁移步骤重写类。**不要求一次改完，也不要求照搬 UE 的全部 RHI 能力。**

### 非目标

- 不在本阶段实现 D3D12 后端。
- 不实现 Render Dependency Graph、并行命令录制、Bindless、异步计算。
- 不要求第一版支持运行时在多个后端之间切换。
- 不把 UE 的 `FRHI*` 类名、描述符布局或着色器系统原样搬进来。
- 不因此阻塞 G1/G2 的场景、Editor 与可玩闭环。

## 2. UE RHI 的参照与取舍

借鉴的规则：

1. **资源不透明**：纹理、缓冲、Shader、PSO、Framebuffer 由 RHI 工厂创建，公共层不接触 `GLuint`、`ID3D12Resource*`、`MTLTexture`。
2. **命令列表**：`BeginRenderPass → SetPipeline → SetBindings → SetViewport/Scissor → Draw → EndRenderPass` 统一记录；OpenGL 后端可立即执行，D3D12 后端录制后提交。
3. **状态成组**：Shader、混合、深度、光栅、顶点输入布局、附件格式组成 `GraphicsPipeline`，不再靠全局 Bind 拼状态。
4. **显式绑定**：常量缓冲、纹理、Sampler 通过固定槽位/描述符绑定，公共接口不提供按名字逐项设值。
5. **统一坐标契约**：viewport/scissor 左上原点像素坐标；纹理 UV 左上原点；`SetData` 行从上到下。各后端内部转换。
6. **生命周期与延迟释放**：资源引用计数；D3D12 在帧栅栏之后释放，公共接口不暴露原生句柄。

不照搬的部分：

- UE 的 RDG、动态 RHI、并行命令列表、Shader 参数反射、Bindless 描述符体系属于更远阶段。
- UE 以 HLSL 为主 Shader 语言并跨编译；Limen 当前 GLSL 仅存在于 OpenGL 后端。**Shader 主语言与跨编译方案单独决策**，不在本设计里拍板。

## 3. 必须写死的契约

### 3.1 坐标与行序

| 项目 | 契约 | OpenGL 后端处理 |
|---|---|---|
| viewport/scissor | 帧缓冲**左上原点**像素坐标 | 内部换算 `glY = height - y - h` |
| 纹理 UV | `v = 0` 对应纹理**顶行** | 采样或上传时翻转 V |
| `SetData` 行序 | 输入数据第 0 行是纹理**顶行** | 上传前翻转行，或后端采样统一翻转 |
| UI 逻辑坐标 | 左上原点、逻辑尺寸 | 由 Editor 后端换算到帧缓冲像素 |

推论：`FontAtlas` 的 UV 必须采用**左上原点**语义。stb 把字形自底向上打包，因此字形在图集中的顶行是 `y1`、底行是 `y0`，FontAtlas 应写 `V0 = y1/H`（顶边）、`V1 = y0/H`（底边）。这是与图形 API 无关的规范化结果，不属于 OpenGL 特化；不得为了 OpenGL 再额外翻转。纹理行序与采样翻转只允许出现在后端内部。

### 3.2 格式与枚举

公共接口只使用 Limen 枚举（`PixelFormat`、`PrimitiveTopology`、`BlendMode`、`CullMode`、`IndexFormat` 等）。禁止在 `LimenEngine/include/Limen/` 中出现 `GLenum`、`GLuint`、`DXGI_FORMAT`、`ID3D12*`、`MTLPixelFormat`。

### 3.3 绑定模型

- 常量数据按固定块绑定：`FrameConstants`、`ViewConstants`、`ObjectConstants`、`MaterialConstants`。
- 纹理与 Sampler 按槽位绑定。
- 公共 RHI 不提供 `SetFloat("u_Color")` 这类按名字设值接口；按名字设值最多保留在具体后端内部，且必须由常量块布局驱动。

### 3.4 资源与生命周期

- RHI 设备创建资源；资源以引用计数句柄暴露（`Ref<RHIResource>` 或等价类型）。
- 谁创建谁持有；Renderer、Material、Mesh 通过 `Ref` 共享，Scene 与面板不拥有 GPU 资源。
- D3D12 必须在帧栅栏之后释放资源；OpenGL 也通过同一套延迟释放接口，避免两套生命周期。
- 公共接口不返回原生资源指针；跨模块借出 GPU 图像时使用不透明的 `RHITextureView`。

### 3.5 后端边界

- OpenGL：VAO、FBO、`glViewport` 的 Y 翻转、纹理行翻转、`glBindTexture` 全部只出现在 `src/RHI/macOS/OpenGL/`。
- D3D12：RTV/DSV、SRV 描述符、命令列表、资源状态屏障全部只出现在 DX12 后端。
- Editor：只通过 `RHICommandList` 与 `RHITextureView` 画 UI 与视口，不直接调用 GL/DX。

## 4. 目标接口（设计级）

命名空间建议 `Limen::RHI`，公共头位于 `LimenEngine/include/Limen/RHI/`。以下是职责与最小方法形状，不是最终签名。

### `RHIDevice`

- 创建资源：`CreateBuffer`、`CreateTexture`、`CreateSampler`、`CreateShader`、`CreateGraphicsPipeline`、`CreateFramebuffer`。
- 创建命令列表：`CreateCommandList`。
- 查询能力：`GetCapabilities()`（格式、MSAA、最大纹理尺寸、特性开关）。
- 提交与呈现：`Submit(commandList)`、`Present(swapchain)`。

### `RHICommandList`

- `BeginRenderPass(const RHIFramebuffer&, const RenderPassSpec&)`
- `EndRenderPass()`
- `SetGraphicsPipeline(const RHIGraphicsPipeline&)`
- `SetVertexBuffer(uint32_t slot, const RHIBuffer&)`
- `SetIndexBuffer(const RHIBuffer&, IndexFormat)`
- `SetUniformBuffer(uint32_t slot, const RHIBuffer&)`
- `SetTexture(uint32_t slot, const RHITexture&, const RHISampler*)`
- `SetViewport(const RHIViewport&)`、`SetScissor(const RHIScissor&)`
- `DrawIndexed(const DrawIndexedArgs&)`
- `Clear(const RHIFramebuffer&, ClearFlags)`

### 资源与描述

- `RHIBuffer`：顶点/索引/常量统一资源，`BufferUsage` 描述用途。
- `RHITexture`：尺寸、像素格式、Mip、MSAA、`SetData`（顶行起点契约）。
- `RHISampler`：过滤、寻址、各向异性。
- `RHIShader`：阶段 + 字节码/源码（具体 Shader 方案单独立项）。
- `RHIGraphicsPipeline`：Shader 阶段、顶点布局、混合、深度、光栅、附件格式、拓扑。
- `RHIFramebuffer`：颜色/深度附件与尺寸。
- `RHITextureView`：不透明纹理视图，用于 Editor 视口把 SceneRenderer 的最终图交给 UI 后端。
- `RHIFence`：提交完成同步与延迟释放。

## 5. 现有类到目标类的映射

| 现有 | 目标 | 迁移说明 |
|---|---|---|
| `RendererAPI` + `RendererCommand` | `RHIDevice` + `RHICommandList` | 静态命令入口保留一版薄包装，逐步改为每帧命令列表 |
| `VertexArray` | `RHIGraphicsPipeline` 的顶点布局 | VAO 留在 GL 后端内部，DX12 用 PSO 输入布局 |
| `Shader` 按名 SetFloat | `RHIShader` + `RHIBuffer` 常量块 | 按 Frame/View/Object/Material 分块 |
| `Texture2D` | `RHITexture` | 明确行序与 UV 契约，原生句柄不公开 |
| `VertexBuffer` / `IndexBuffer` / `UniformBuffer` | `RHIBuffer` | 用 Usage 区分，DX12 映射同一资源类型 |
| `GraphicsPipeline` | `RHIGraphicsPipeline` | 现有职责接近，补齐输入布局与附件格式 |
| `Framebuffer` + `RenderPass` | `RHIFramebuffer` + `BeginRenderPass` | RenderPass 退化为描述结构 |
| `uintptr_t` 视口纹理 | `RHITextureView` | Editor 不再持有 GL Texture ID |
| `SetViewport/SetScissor` | `RHICommandList` 方法 | 坐标契约改为左上原点 |

## 6. 迁移顺序与验收

迁移按"保持可运行"的顺序推进；每一步都要通过现有 CPU/GPU 回归。

1. **文档契约（本文）。** 验收：契约无歧义，团队确认。
2. **引入 `RHICommandList`。** 先把 Clear/Viewport/Scissor/Draw 迁入；GL 后端立即执行，`RendererCommand` 做薄包装。验收：Editor 裁剪测试、Renderer2D 管线、后处理读回不变。
3. **纹理/缓冲契约落地。** `SetData` 顶行契约、UV 左上原点；`FontAtlas` 保持 `V0=y1/V1=y0`（字形顶边在前）；GL 后端统一翻转行序与采样。验收：字体方向测试在 GL 通过，且 FontAtlas 不再含任何 GL 特化。
4. **PSO 取代 `VertexArray`。** 顶点布局进 PSO，VAO 只存在于 GL 后端。验收：现有 3D 场景与样例模型画面不变。
5. **常量块与固定槽位。** 取代按名字 uniform。验收：材质、相机、光照参数读回与画面不变。
6. **`RHITextureView` 取代 `uintptr_t`。** Editor 视口只持有不透明视图。验收：视口渲染、2x DPI、直通贴图测试不变。
7. **D3D12 后端。** 按本文契约实现同一 RHI；验收：相同场景与 UI 在 DX12 下画面一致，字体方向与裁剪正确。

## 7. 总体验收

- 公共头文件不出现任何图形 API 原生类型（可用 grep 检查）。
- Editor 与 SceneRenderer 不直接调用 OpenGL/D3D12。
- 同一字体图集、同一 UI 绘制在 OpenGL 与 D3D12 下方向一致。
- 同一 scissor 矩形在两种后端裁出相同的像素区域。
- GPU 资源在 Context/Device 销毁前释放；DX12 延迟释放有帧栅栏保障。

## 8. 待决定

- Shader 主语言：继续每后端各写一份，还是采用 HLSL 主源 + 跨编译。
- 命令列表是否引入多线程录制（第一版建议单线程）。
- 描述符模型：固定槽位先行走通，Bindless 只作为研究项。
- 统一资源类型是否在本阶段直接重命名，还是先加包装层再逐步替换。
- DX12 资源状态：第一版手工 barrier，还是做轻量状态跟踪。

## 9. 与当前里程碑的关系

本设计不阻塞 G1/G2。建议在第 3 步（纹理/UV 契约）和第 2 步（scissor/viewport 契约）随下次 UI/渲染改动落地；D3D12 后端属于 P 路线，在 macOS 可玩闭环稳定后再启动。
