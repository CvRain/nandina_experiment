# 图片 / 纹理子系统（Image & Texture）

> 状态：Stage 1–3 已完成（RGBA 纹理加载 + raylib 后端 + `widget::Image` 的 source_rect 裁剪、
> stretch/contain/cover、contain 对齐、crop/resize/tint 预处理，以及示例接入一张品牌图）。
> 纹理抽象已在 glyph atlas 阶段落地（仅 alpha 单通道），本子系统补上 RGBA 图片加载与 Image 节点，
> 形成「外部图片 → 渲染设备纹理 → 节点绘制」闭环。
> 关联：README 路线图 1.1（图片嵌入）；render 层 `IRenderDevice` 抽象。

## 1. 目标

1. 让框架能加载并绘制一张外部图片（PNG/JPEG 等），成为 2D 游戏引擎式 UI 的 sprite 能力。
2. 复用 raylib 的 `Image`/`Texture` 能力（`LoadImage` → 处理 → `LoadTextureFromImage` →
   `DrawTexturePro`），但 raylib 类型绝不进公共接口（沿用 render 层约束）。
3. 加载方式先用文件路径（相对路径），nanres 资源管理器集成明确延后（见 §6）。

## 2. 现状

- `IRenderDevice` 已有 `TextureHandle` 与 `create_alpha_texture / update_alpha_texture /
  destroy_texture / draw_texture_region`，当前仅服务 FreeType glyph atlas（alpha 单通道，点采样）。
- raylib 后端 `raylib_device.cpp` 已维护 `unordered_map<handle, Texture2D>`：
  `LoadTextureFromImage` 上载、`DrawTexturePro` 绘制、析构统一 `UnloadTexture`。
- 因此本子系统是「在既有纹理抽象上补 RGBA 加载 + 尺寸查询 + 节点」，不是新建第二套纹理体系。

## 3. 设计决策

### 3.1 加载方式：文件路径（nanres 延后）

Stage 1–3 一律用文件路径加载图片：

```cpp
auto handle = device.load_texture_from_file("assets/hero.png");
```

不做 nanres 集成。原因（与开发者体验一致）：

- 项目本体不够成熟，nanres 尚无消费方，无法端到端验证。
- 当前没有 QML 式资源导入（`qt_add_qml_module()` / `qt6_add_resources()` 一类），开发者要
  先构建 nanres、把 nanres 的 meson 配置引入项目、再在代码里注册资源，链路比「编译后
  把资源文件放输出目录、用相对路径」更繁琐。
- nanres 作为可扩展性能力保留；等它具备等价的一键资源导入方式后再接入（届时只增一个
  `load_texture(bytes, ...)` 桥接即可，不影响本子系统 API 形状）。

### 3.2 IRenderDevice 纹理 API 扩展

在既有 `TextureHandle` 基础上增加两个接口（默认 no-op，录制设备可覆写计数）：

```cpp
// 从文件路径加载 RGBA 纹理；失败返回空 handle。
[[nodiscard]] virtual auto load_texture_from_file(std::string_view path) -> TextureHandle {
    return {};
}
// 查询纹理自然像素尺寸；无效 handle 返回 {0,0}。
[[nodiscard]] virtual auto texture_size(TextureHandle texture) -> NanSize {
    return {};
}
```

复用既有 `draw_texture_region(handle, source, destination, tint)` 绘制、`destroy_texture`
释放。raylib 后端用 `LoadTexture(path)`（Stage 1）或 `LoadImage + 处理 + LoadTextureFromImage`
（Stage 3）实现；RGBA 图片用双线性过滤（与 glyph atlas 的点采样区分，避免图片缩放锯齿）。

### 3.3 Image 节点

新增 `widget::Image`——一个 `NanControl`（参与布局）：

- `set_source(path)`：记录路径，进入树后（拿到设备）懒加载纹理。
- 自然尺寸：默认取 `texture_size(handle)`，可 `set_size` 覆盖。
- `set_source_rect(NanRect)`：裁剪（九宫格/atlas 后续）。
- `set_tint(NanColor)`：着色；per-node opacity（单元 5）自动乘入。
- `on_draw`：`device.draw_texture_region(handle, source, dest, tint × ctx.opacity())`。

命名：UI 语义用 `widget::Image`；若后续要游戏侧自由变换（旋转/缩放/锚点）的 sprite，再单独立
`scene::Sprite`（NanNode2D），不与布局耦合。

### 3.4 生命周期与设备上下文

纹理句柄由渲染设备拥有（与 `FontPipelineCache` 同模式，device 析构统一释放）。Image 节点
只持有 handle，不负责释放；退出树不清纹理（缓存友好，后续可加按路径去重的纹理缓存）。

设备上下文获取沿用 R9 的 pattern：场景树携带设备级纹理上下文，Image 节点在进入树时经
场景树上下文解析设备并懒加载；避免在 `on_draw` 内做加载副作用。

## 4. 分阶段交付

| 阶段 | 交付 | 验收 |
|---|---|---|
| 1（完成） | `load_texture_from_file` + `texture_size` + raylib 后端 + 最小 `widget::Image` | 测试图绘制、尺寸/tint/opacity 生效；RecordingDevice 覆盖调用序 |
| 2（完成） | Image 能力补全（source_rect 裁剪、缩放/对齐模式） | 裁剪/缩放/对齐测试 |
| 3（完成） | 图片调整（resize/crop/tint，raylib `Image` 处理）+ 示例接入 | 示例展示图片 + 调整；全量绿 |

## 5. 验收条件（Stage 1）

- 加载 PNG，正确绘制到目标矩形；尺寸/tint/opacity 生效。
- 无效路径/无效 handle 安全降级（不绘制、不崩溃）。
- `RecordingDevice` 覆盖 `load_texture_from_file` / `texture_size` / `draw_texture_region` 调用序。
- 全量 meson test 保持绿（新增用例后相应增加）。

验证（headless）：`image_tests` 覆盖懒加载一次、源/目标矩形、自然尺寸、加载失败降级、
tint × per-node opacity；全量 meson test 41/41。图片实际渲染待 Stage 3 示例接入后视觉复核。

Stage 2 补充：`set_source_rect` 裁剪纹理源区域；`set_scale_mode(stretch/contain/cover)` 与
`set_alignment(start/center/end)`（仅 contain 生效）。contain 等比缩放并居中/贴边，cover 按
目标纵横比裁剪源区域居中。`image_tests` 覆盖 source_rect、contain 居中与 start/end 贴边、
cover 裁剪；41/41。

Stage 3 补充：`IRenderDevice::load_texture_from_file(path, ImageLoadOptions)` 增加可选的
crop/resize/tint 预处理（raylib `ImageCrop`/`ImageResize`/`ImageColorTint`，均为 CPU 侧），
`widget::Image::set_load_options` 透传并按需重载。示例 About 页接入一张品牌图（`assets/nandina_logo.png`
经 meson `configure_file(copy)` 复制到可执行文件同目录，按相对路径加载）。`image_tests` 覆盖
load options 透传；41/41。图片真实渲染视觉复核需在有显示环境运行示例。

## 6. 后续：nanres 集成（延后）

当 nanres 具备 QML 式一键资源导入（或至少「项目内声明资源清单 + 构建自动打包 + 运行时按
逻辑键加载」的顺畅链路）后，再增加 `load_texture(bytes, ...)` 桥接，把 `res://image.png`
这类逻辑键接到 ResourceManager → 字节 → 设备纹理。本阶段 API 形状保持不变。
