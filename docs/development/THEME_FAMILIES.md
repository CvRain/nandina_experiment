# 内置主题族（Theme Families）

> 状态：✅ butter 已落地（基础设施 + 示例抽离完成）；fluent/material 待常用组件补齐后再做
> 关联：Phase 8 Deferred Work「built-in theme families」
> 参考：[Fluent 2](https://fluent2.microsoft.design/design-tokens)、
> [Material 3 色彩系统](https://m3.material.io/styles/color/roles)、
> [Catppuccin palette](https://catppuccin.com/palette/) 与
> [Catppuccin style guide](https://github.com/catppuccin/catppuccin/blob/main/docs/style-guide.md)、
> 本地 `awesome-design-md/clay` 分析。

## 1. 目标

1. 把目前写死在 `example/settings_example.cpp` 里的 Catppuccin 品牌主题抽离成
   **框架内置主题族**，由 `ThemeManager` 统一注册与切换。
2. 内置几套「平台风格」主题，方便同一应用在不同平台上采用贴近原生观感的默认外观。
3. 保留一套有项目自身识别度的签名主题——**黄油卡片基调 + Catppuccin 配色**（`butter`）。

## 2. 主题族模型

沿用既有事实来源链：`NanReferencePalette`（7 组 11 档色阶）+ `PaletteVariantPolicy`
（亮/暗品牌档位）→ `make_color_scheme()` → `DesignSystem.light / dark`；组件只消费语义色。
因此**一个主题族 = 一组参考色阶 + 一个变体策略 + 一组 tokens**，复用完 `default_design_system()`
的 typography 与 recipe，只替换 light/dark 语义色与 tokens。

```text
ThemeFamilyDefinition { name, reference, policy, tokens }
        -> build_family_design_system()   // = default_design_system() 拷贝，替换 tokens + light/dark
        -> ThemeManager 注册为「全快照主题族」
        -> activate_family(name) 原子 apply；set_preference() 在族内翻转 light/dark
```

- 参考色阶的**单条 11 档 ramp 覆盖亮↔暗**：`make_color_scheme` 对 light 取浅端、
  对 dark 取深端（这正是当前 example 的做法）。品牌色 950 档放 flavor base，实现
  Catppuccin「On Accent = Base」规则。
- 框架默认 `default_design_system()`（Skeleton）保持不变，作为 detached widget / 测试的
  中性回退；应用通过 `activate_family()` 显式选择主题族，`butter` 为 example 的默认选择。

## 3. 内置主题族

### 3.1 `butter` —— 黄油卡片 + Catppuccin（项目签名，example 默认）✅

- 设计语言：奶油暖底（奶白/米黄）、琥珀暖橙主色、大圆角卡片，温暖友好（对齐 Clay 的
  cream canvas 思路，配色以 Catppuccin 为参考做暖调微调）。
- 中性 ramp（暖奶油 → 暖棕，11 档；950 档即「On Accent = Base」的 base）：
  `#faf6ec #f2ead9 #e8dcc4 #d9c8a8 #b3a184 #96856a #7a6b54 #5a4f3e #3f372b #2a241b #1c1811`
- primary（琥珀暖橙，参考 Peach/Rosewater，950 档 = base）：
  `#fff6e2 #ffebc4 #ffd997 #fcc36c #f5a845 #e78d28 #c97217 #a15710 #7a3e0a #522804 #1c1811`
- tertiary（暖粉，参考 Flamingo/Maroon）：
  `#f9e4e0 #f4cccc #eba5ae #dd7f8a #d45a68 #c23a4e #a62a3c #841f2f #61151f #3e0c12 #24060a`
- secondary / success / warning / error 沿用 `default_reference_palette()`（Skeleton OKLCH）。
- policy：`{ light_brand = shade_500, dark_brand = shade_300 }`。
- tokens：`radius sm/md/lg = 12/16/24`，`border thin/medium = 2/3`（软阴影待 shadow 图元）。

### 3.2 `fluent` —— Fluent 2 / Windows

- 设计语言：冷灰中性、直角偏小圆角（4/8px）、8px 间距网格、Segoe UI 质感。
- 中性：亮底 `#f3f3f3`、surface `#ffffff`、文字 `#242424`；暗底 `#202020`、surface `#2b2b2b`、
  文字 `#ffffff`（生成 11 档 ramp）。
- 强调色（accent）：light `#0067c0`（Fluent Blue）、dark `#4cc2ff`（亮青蓝）。
- 状态色：success `#107c10`、warning `#9d5d00`、error `#c42b1c`（暗色变体更亮）。
- policy：`{ light_brand = shade_500, dark_brand = shade_400 }`。
- tokens：`radius sm/md/lg = 4/8/12`，`border thin/medium = 1/1`。

### 3.3 `material` —— Material 3 / Android

- 设计语言：Material 3 色调、surface/on-surface 分层、4px 密度、全圆角。
- 中性：亮底 `#fef7ff`、surface `#fffbfe`、文字 `#1c1b1f`；暗底 `#1c1b1f`、文字 `#e6e1e5`。
- primary：light `#6750a4`（紫）、dark `#d0bcff`；secondary `#625b71 / #ccc2dc`；
  tertiary `#7d5260 / #efb8c8`。
- 状态色：success `#2e7d32`、warning `#b26a00`、error `#c62828`（暗色更亮）。
- policy：`{ light_brand = shade_500, dark_brand = shade_400 }`（Material 式暗色提亮品牌档）。
- tokens：`radius sm/md/lg = 8/12/16`，`border thin/medium = 1/2`。

## 4. ThemeManager 集成

新增「全快照主题族」注册路径（与既有 light/dark 命名族并存）：

```cpp
void ThemeManager::register_theme_family(std::string name, DesignSystem system); // 或 shared_ptr
// activate_family(name) 优先查全快照族，找不到再回退 legacy light/dark 命名族
```

`activate_family(name)` 原子 apply 全快照；`set_preference(light/dark/system)` 仅翻转
`appearance()`（DesignSystem 内嵌两套 palette 已支持）。

## 5. Example 迁移

1. 删除 example 内的 `brand_design_system()`（约 40 行），改用
   `themes.register_theme_families(default_theme_families()); themes.activate_family("butter")`。
2. Appearance 单选组继续驱动 `set_preference`（System/Light/Dark）。
3. 新增「主题族」单选组（Butter/Fluent/Material）驱动 `activate_family`，作为真实切换验证。

## 6. 测试

- 每族 `build_family_design_system` 生成 light/dark 语义色，背景/文字明度翻转且
  `|ΔL| ≥ 0.25`（复用 contrast 工具）。
- `register_theme_family` + `activate_family` 后 widget 跟随新快照（revision 只发布一次）。
- `set_preference` 在族内翻转，`activate_family` 切换族后 preference 保持。
- 全快照族与 legacy 命名族并存、互不破坏（回归现有 theme_manager 测试）。

## 7. 提交边界（依赖序）

1. `feat(theme)`: `builtin_themes.{hpp,cpp}`（`default_theme_families()` + 三族色阶/token/policy）
2. `feat(theme)`: `ThemeManager::register_theme_family` 全快照族路径
3. `test(theme)`: 主题族解析/切换/对比测试
4. `feat(example)`: example 抽离品牌主题 → `activate_family("butter")` + 主题族切换 UI
5. `docs(theme)`: 本文档 + WORKFLOW 现状同步
