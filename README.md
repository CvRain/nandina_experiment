# Nandina v3（南天竹）

> 一个 C++26 + raylib 的「2D 游戏引擎式 UI 框架」：Godot 式场景树为底座，
> Flutter/Angular 式声明式响应式编写体验，shadcn 式 primitives/tokens/theme 设计体系。

NandinaUI 把现代前端的成熟范式带到原生 UI 开发，当前是 v3（C++）主线：
旧 Zig/C++ 主线归档在 `dev-docs-v3/NandinaUI/`，仅作语义参照。

## 设计理念

分层而非「DSL-first」。命令式 widget API 是运行时契约；声明式作者语法只是在其上
创建、组合、绑定并返回同一批具体控件，不引入第二套对象模型/渲染器/状态引擎/生命周期。
开发者始终能持有 widget 引用、从树/router 取回节点、调用普通 setter，或下沉到 primitive 自绘。

```text
Authoring DSL / builders
Declarative bindings（If / ForEach / keyed reconciliation）
Semantic widgets + application style rules
Primitives + tokens
NanControl / NanNode scene tree
Layout, input, text, render device, platform window
```

- 一条事实来源：`resources.toml` 是唯一人工资源清单，锁/包数据与 Meson 接线由它派生。
- 语义与交付分离：resource 层管字体字节，font registry 管逻辑 family/face/fallback，
  style 层决定组件请求哪个 family。
- token 承载主题相关值，literal 承载刻意固定覆盖；不把解析后的主题值写回本地组件样式。
- 零 RTTI 优先：用 `as_node2d()` / `as_control()` / `layout_flex_factor()` 等虚钩子，
  raylib 不进入公共接口。

## 架构分层（`nandina/`）

| 层 | 职责 |
|---|---|
| `foundation/` | 颜色（OKLCH）、几何、变换、对比度、UTF-8、日志 |
| `reactive/` | `Signal`/`Computed`/`Effect`/`Event`/`Property`/`ReactiveScope`（Angular 式） |
| `scene/` | `NanNode`/`NanNode2D`/`NanControl`、场景树、命中测试、输入路由、z 序、布局 |
| `render/` | 可替换渲染设备抽象、DrawContext、clip/opacity |
| `resource/` | 资源管理器、多后端（builtin/directory/sqlite）、扫描、包清单 |
| `text/` | FreeType 字面、HarfBuzz 整形、字形图集、字体 family/fallback、系统字体发现 |
| `theme/` | tokens、语义色板、配方书（recipe/rule/resolve）、主题族、ThemeManager |
| `widget/` | 语义组件 + primitive（box/shadow/focus/ripple/text/pressable）+ 布局 |
| `semantics/` | 可访问性树（Role/State/Action） |
| `app/` | NanApplication/NanWindow/NanRouter（keep-alive 页栈）/Page/Store |
| `animation/` | easing、`Tween<T>`、`Behavior<T>`、`AnimatedProperty<T>`、typed endpoint、场景树 `AnimationHost` |

## 已实现能力

- **场景树**：节点生命周期、变换、父子/兄弟 z 序（`subtree_z_index_hint` 让浮层盖住后续兄弟）、
  命中测试、焦点遍历、键盘/鼠标路由。
- **响应式**：`ui.signal/computed/effect/bind/connect`，`if/when`、`for_each` 键控调和。
- **组件**：Button、Checkbox、Slider、Switch、TextField、Badge、Card、ProgressBar、
  RadioButton/RadioGroup、Tabs、Tooltip、Select、Divider、Avatar、Chip、**Dialog**（遮罩+
  焦点陷阱+全屏定位）。
- **主题系统**：三套内置主题族 `butter`（黄油/Catppuccin 签名）、`fluent`（Fluent 2）、
  `material`（Material 3，精确对齐 M3 baseline）；`on_primary` 前景字随外观翻转；
  token + 配方书 + 规则覆盖 + 每组件 `set_override`。
- **软阴影**：Card 软阴影（butter 族 claymorphism 抬升）。
- **路由**：`NanRouter`（keep-alive 页栈 + push/pop/replace + 参数页 + 生命周期）；
  示例用「侧边栏 + 嵌套 router」的 settings dashboard 承载。
- **动画**：easing + `Tween<T>`；`Behavior<T>` 与 `AnimatedProperty<T>` 完成 target/value
  值语义；typed property endpoint 将 Builder `.bind/.behavior`、命令式 setter 与场景树
  `AnimationHost` 汇入同一 target，并在 reactive 与 layout 之间只推进活跃轨道。
  Tabs 下划线与 Dialog 淡入已迁移到 Host；`spring`/`keyframes`/`motion::` DSL、组合器
  （parallel/sequential/stagger）与 router 页面转场也已落地，全局 reduced-motion 由 Host 统一。
- **字体**：系统字体发现 + CJK 回退（Noto Sans CJK / 思源黑体 / Sarasa / 苹方 / 微软雅黑 /
  文泉驿…）；`FontLoader::load_file`（任意路径）+ `register_face` 同族多字重/斜体变体 +
  `find_system_font`（在 `/usr/share/fonts` 按名取路径），多 family 可按组件/实例分配字体。
- **图片**：`widget::Image` 文件路径加载 + 裁剪/缩放/对齐 + raylib 预处理（crop/resize/tint）。
- **可访问性**：semantics 树（Role/State/Action），全部交互组件具备键盘可达 + 焦点环。

## 示例

`example/settings_example/` —— 一个 router 驱动的 settings dashboard：
- `SettingsStore` 承载跨页状态，`ShellPage` = 侧边栏 + 嵌套 `NanRouter`。
- General / Appearance / Components / About 四页 + 带参数 DetailPage（push/pop）。
- 完整展示所有组件、主题族切换（butter/fluent/material）、外观亮暗切换、
  Reset 确认对话框、CJK 下拉（中文/日本語）。

## 构建与测试

```sh
meson setup buildDir
meson compile -C buildDir
meson test -C buildDir --no-rebuild          # 45/45（physics2d enabled）
./buildDir/example/settings_example/nandina_settings_example
```

依赖经 Meson subprojects 拉取：raylib、spdlog、freetype、harfbuzz、fribidi、utf8proc、
sqlite3、tomlplusplus、nlohmann/json、box2d（可选）、Catch2。C++26（clang/gcc）。

## 当前状态与路线图

- **1.0（收尾中）**：核心引擎（场景树/响应式/组件/主题/路由/文本/可访问性）全部落地；
  当前完整配置 `tests/` 45/45 绿。资源交付工具链 `nanres`（扫描/校验/锁/打包/portable staging）R0–R10 完成，
  D2（运行时 build-tree 包定位 + `[[resources]]` per-resource 覆盖）完成；D3 原型已具备
  `nandina new`/`build`/`run`/`doctor`。当前 C2.1 正将 CLI 拆分到平行的 `NandinaCLI`
  仓库；registry/archive 获取、分层软件源、镜像、SDK 锁、校验缓存、Meson wrap 以及
  `build`/`run`/`doctor` 已在独立仓库落地；Git fork 固定提交、显式 path 开发模式和
  source 增删启停、Ed25519 签名索引、离线重验及 POSIX/Windows 平台后端也已完成，
  C2.1 代码合同闭合。Windows/macOS 原生支持声明继续由 D4 CI 决定。C6 的 CI、源码归档和
  portable/offline 验证已完成，C6.1 发布元数据对齐已完成；1.0 的最低承诺是 Linux + C++26 + Meson/Ninja + Git submodule；完整验收门禁见
  `docs/development/1.0_ACCEPTANCE.md`，正式仓库晋升流程见
  `docs/development/1.0_PROMOTION_PLAN.md`。
- **1.1（进行中）**：声明式可定制动画系统（per-node opacity、组合器、spring/keyframes、
  `motion::` DSL、router 转场）、图片/纹理子系统、slider value label、自定义字体/多字体导入 +
  `find_system_font` 均已完成；i18n 语言包按约定延后到 1.0 之后。
- **1.2（规划）**：路由转场细化、音频（raylib audio）；Vulkan/SDL 后端评估。

## 文档索引

- `docs/development/README.md` —— v3 开发入口（架构约束、设计哲学）。
- `docs/development/WORKFLOW.md` —— 当前进度快照与后续项。
- `docs/development/1.0_ACCEPTANCE.md` —— Linux 1.0 支持边界、门禁与收尾顺序。
- `docs/development/1.0_PROMOTION_PLAN.md` —— 晋升正式仓库、归档 1.x 与 2.0 分工。
- `docs/development/RELEASE_METADATA.md` —— 版本、CLI 兼容性、支持边界与发布元数据契约。
- `docs/development/NANDINA_CLI_DISTRIBUTION.md` —— 独立 CLI、软件源、SDK 锁与制品合同。
- `docs/development/THEME_FAMILIES.md` —— 主题族设计。
- `docs/development/SOFT_SHADOWS.md` —— 软阴影设计。
- `docs/development/ANIMATION.md` —— 动画系统设计（1.1 施工图）。
- `docs/development/PHASE8_RENDER_THEME_PLAN.md` —— Phase 8 计划与决策。
- `docs/development/IMAGE_TEXTURE.md` —— 图片/纹理子系统设计（1.1，文件路径加载、nanres 延后）。
