# 图片 / 纹理子系统（Image & Texture）

> 状态：Stage 1–4 已完成；Stage 4（C3）连接打包资源字节与图片纹理加载。
> 已有能力包括 RGBA 纹理加载 + raylib 后端 + `widget::Image` 的 source_rect 裁剪、
> stretch/contain/cover、contain 对齐、crop/resize/tint 预处理，以及示例接入一张品牌图。
> 纹理抽象已在 glyph atlas 阶段落地（仅 alpha 单通道），本子系统补上 RGBA 图片加载与 Image 节点，
> 形成「外部图片 → 渲染设备纹理 → 节点绘制」闭环。
> 关联：README 路线图 1.1（图片嵌入）；render 层 `IRenderDevice` 抽象。

## 1. 目标

1. 让框架能加载并绘制一张外部图片（PNG/JPEG 等），成为 2D 游戏引擎式 UI 的 sprite 能力。
2. 复用 raylib 的 `Image`/`Texture` 能力（`LoadImage` → 处理 → `LoadTextureFromImage` →
   `DrawTexturePro`），但 raylib 类型绝不进公共接口（沿用 render 层约束）。
3. 普通文件路径保持兼容；正式应用资源通过 `res://<ResourceKey>` 从 nanres 包加载。

## 2. 现状

- `IRenderDevice` 已有 `TextureHandle` 与 `create_alpha_texture / update_alpha_texture /
  destroy_texture / draw_texture_region`，当前仅服务 FreeType glyph atlas（alpha 单通道，点采样）。
- raylib 后端 `raylib_device.cpp` 已维护 `unordered_map<handle, Texture2D>`：
  `LoadTextureFromImage` 上载、`DrawTexturePro` 绘制、析构统一 `UnloadTexture`。
- 因此本子系统是「在既有纹理抽象上补 RGBA 加载 + 尺寸查询 + 节点」，不是新建第二套纹理体系。

## 3. 设计决策

### 3.1 加载方式：文件路径与打包资源

Stage 1–3 用文件路径加载图片：

```cpp
auto handle = device.load_texture_from_file("assets/hero.png");
```

Stage 4 保留该接口用于开发期文件和兼容已有代码，同时增加：

```cpp
auto image = ui.make<widget::Image>("res://images/hero.png");
```

`BuildContext` 将页面已有的 `ResourceManager` 非 owning 服务注入 `Image`。首次绘制时，
`Image` 解析 `res://` URI、按稳定 `ResourceKey` 获取不可变资源句柄，并把字节与 media type
传给渲染设备。缺少服务、资源不存在、URI 非法或格式不受后端支持时安全返回空纹理，且不逐帧
重试。其他不带 `res://` 前缀的字符串仍按文件路径加载。

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
| 4（C3，完成） | `ResourceManager/package bytes → Image → texture`，示例改用 nanres 包 | `res://` 加载、服务注入、失败降级和真实打包示例均有自动化覆盖；42/42 |

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

## 6. Stage 4：nanres 集成（C3）

`IRenderDevice::load_texture_from_memory(bytes, media_type, options)` 是文件加载的并行入口；
raylib 根据 media type 选择解码扩展名，调用 `LoadImageFromMemory` 后复用相同的
crop/resize/tint 与纹理上传流程。Settings 示例的品牌图进入 `resources.toml` 管理的包，
`NanApplicationConfig::for_process` 通过可执行文件旁的 `resource-location.json` 挂载 build-tree
包，About 页使用 `res://nandina_logo.png`，不再依赖复制到输出目录的散文件。
构建配置显式启用 raylib 的 `SUPPORT_FILEFORMAT_JPG`，因此 `image/jpeg` 资源与 PNG 一样可在
内存路径中解码；下游自定义 raylib 构建也必须保留该选项才能支持 JPEG。

验证：`image`、`application-resource`、`settings-example` 聚焦测试通过；完整
`meson compile -C buildDir` 与 `meson test -C buildDir --print-errorlogs` 通过（42/42）。
