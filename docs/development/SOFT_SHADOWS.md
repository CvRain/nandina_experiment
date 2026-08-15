# 软阴影（Claymorphism Elevation）

> 状态：✅ 已落地（render 图元 + ShadowPainter + butter 卡片阴影）
> 关联：PHASE8 Deferred Work「soft UI shadows」；butter 主题族卡片的 elevation 落地

## 1. 目标

给渲染设备新增「软阴影」图元，让 butter 等主题能给卡片加轻微 elevation，且不需要
per-widget 纹理离屏模糊。Clay 风格（奶油卡片 + 柔和阴影）依赖此能力。

## 2. 图元设计

在 `IRenderDevice` 新增（默认 no-op，录制设备按需覆写计数）：

```cpp
/// 软阴影：圆角矩形 + 软边衰减（spread 越大越柔和），中心不透明、边缘渐隐。
virtual void draw_rounded_rect_shadow(
    const foundation::NanRect& rect,  // 世界坐标（已含偏移）
    float radius,                     // 圆角
    float spread,                     // 软边衰减宽度（>0）
    const foundation::NanColor& color // 阴影颜色（带 alpha 控强度）
) {}
```

- 阴影不参与 hit-test / layout，只影响绘制（与 BoxPainter 同一层级）。
- 默认 no-op，保证既有录制设备无需改动即可编译。

## 3. raylib SDF 后端

在现有 SDF shader 增 `uMode == 4`（shadow）：

```glsl
// alpha = uColor.a * (1.0 - smoothstep(0.0, spread, sdRoundRect(...)))
```

用 `spread` 替换现有 1 像素 `fwidth` 过渡，得到可控软边。raylib 侧复用同一 `aa_shader_`
与 white texture，仅新增 uniform 传 spread（复用 uRadius.y 或新增）。

## 4. Theme 数据

- 新增共享片段 `ShadowStyle { ThemeColor color; ThemeScalar offset_x; ThemeScalar offset_y; ThemeScalar spread; }`
  （offset_x/offset_y 为「鼠标为中心的光照/阴影变化」预留；Clay 双阴影后续按需扩展）。
- 挂到 `CardRecipe`（`ResolvedCardStyle` 增 `shadow`），默认配方为无阴影（透明 + spread 0）；
  `ThemeFamilyDefinition.card_rules` 让主题族按需注入阴影规则（butter 注入暖棕软阴影）。

## 5. Painter + Widget

- 新增 `ShadowPainter::paint(ctx, rect, radius, shadow, opacity)`：先画阴影再画容器。
- `Card::on_draw` 在 `BoxPainter::paint` 之前画阴影；`butter` 默认配方给 Card 配一处柔和阴影。

## 6. 提交边界（依赖序）

1. `feat(render)`: IRenderDevice 增 `draw_rounded_rect_shadow`（默认 no-op）+ raylib SDF mode 4
2. `feat(theme)`: ShadowStyle 片段 + CardRecipe/ResolvedCardStyle + butter 默认阴影
3. `feat(widget)`: ShadowPainter + Card 绘制阴影
4. `test(render/widget)`: 录制设备断言阴影先于容器绘制
5. `docs(theme)`: 本文档 + WORKFLOW 现状
