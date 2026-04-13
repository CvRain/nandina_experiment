# 2026-04-10 设计讨论纪要

本文件整理自 `fri_apr_10_2026_design_discussion_for_ui_framework.json`，用于沉淀已确认的设计决定、取舍理由，以及它们在当前仓库中的落地状态。

---

## 1. 会议背景

- 目标不是再造一个“Qt 风格 widget 库”，而是探索一个更接近前端框架体验的现代 C++ 桌面 UI 框架。
- 核心诉求是零宏、组合式组件、响应式状态、跨平台渲染，以及未来可承载列表、树、文件管理器这类复杂 UI。
- 讨论过程中曾出现过早期命名与多种架构分支，本纪要仅保留后来被确认的主线。

---

## 2. 已确认的架构决定

### 2.1 项目定位

- 项目名称统一为 Nandina，早期的 NovaUI 方案已废弃。
- 框架方向是“现代 C++ 桌面端的前端式开发体验”，而不是 QObject / 宏 / 代码生成路线。

### 2.2 组件模型

- 采用共享组件基类 + 可组合行为单元的方向，而不是深继承组件树。
- 组件行为应优先通过组合扩展，避免 Button / Input / Dialog 等组件逐步演变成不可维护的大型基类体系。
- Props 边界应明确区分拥有端与消费端：父组件拥有可写状态，子组件默认只读消费。

### 2.3 响应式模型

- `State<T>` 负责单值状态、自动依赖追踪、`Computed` 派生值与 `Effect` 副作用。
- `StateList<T>` 负责结构化集合更新，保留 `Inserted / Removed / Updated / Moved / Reset` 语义。
- `StateList<T>` 不应伪装成普通 `State<std::vector<T>>`；需要粗粒度派生时，提供显式桥接 API 即可。
- 组件输入优先使用 `ReadState<T>` 与 `Prop<T>`，避免把可写 `State<T>` 向下暴露。

### 2.4 布局与渲染路线

- 初期先以自实现的 Row / Column / Stack 跑通组件树、布局与 demo 页面。
- Yoga 保留为后续布局引擎接入目标，但不在响应式与基础组件未稳定前提前引入。
- 渲染后端策略保持为 WebGPU / Vulkan 优先，其次 OpenGL，再次软件后端。

### 2.5 工程策略

- 基础层先稳定 Reactive、Core、Layout，再扩展更多高级组件。
- 文档要区分“已实现能力”“近期目标”“历史讨论记录”，避免设计文档和问题清单互相覆盖。

---

## 3. 当前代码库中的落地情况

### 3.1 已落地

- `src/reactive/Reactive.ixx` 已成为唯一的响应式实现入口，包含 `State<T>`、`ReadState<T>`、`Computed`、`Effect`、`EffectScope`、`Prop<T>`、`StateList<T>`、`EventSignal` 与 `batch(...)`。
- `ReadState<T>` / `Prop<T>` 已进入组件输入模型，`Label` 与示例页面均使用只读绑定路径。
- `StateList<T>` 已提供 `version()`、`tracked_size()`、`tracked_empty()`、`tracked_items()` 作为显式桥接接口。
- 最小批量更新 `batch(...)` 已实现，覆盖 `State<T>` 观察者去重与外层批次统一 flush。
- `EventSignal::emit()` 已具备“继续执行其余回调，最后重抛首个异常”的行为语义。
- `example/pages/DemoPage.ixx` 已被改造成真实的列表选择场景，用于同时验证 `StateList<T>`、`tracked_*` 与 `batch(...)`。
- `tests/reactive_smoke.cpp` 已覆盖响应式基础语义、异常路径、组件输入模型、StateList 桥接与 batch 行为。

### 3.2 尚未落地

- Yoga 尚未接入，当前布局仍以自实现容器为主。
- Theme / Token / Variant 仍处于后续里程碑。
- FreeType + HarfBuzz 的完整文字栈尚未进入稳定实现阶段。
- 更丰富的组件库仍在后续路线中。

---

## 4. 与现有文档的对照结论

- `design.md` 应作为当前架构总览，保留稳定结论，不再承载历史草案。
- `development-plan.md` 应反映当前实际进度，而不是保留初始化阶段的里程碑状态。
- `reactive-design.md` 与 `reactive-primitives.md` 需要以“已实现的 State / StateList / batch 关系”为准，不再把 batch 写成纯提案。
- `report-1.txt` 仍保留为历史审查记录，但不再代表当前状态。

---

## 5. 本轮整理后的文档策略

- 保留原始 JSON 作为讨论原始材料。
- 新增本纪要文件，作为人可读的设计决策摘要。
- 删除已经明显过期、且内容已被主文档或当前实现覆盖的草稿文档。
- 后续新增文档时，优先放入“设计总览 / 使用说明 / 历史记录”三类之一，避免继续形成平行版本。

---

## 6. 当前建议的后续工作

1. 继续把 Reactive 1.0 的边界写实化，重点是线程模型、batch 与 `StateList<T>` 的非目标边界。
2. 用更多真实场景验证布局容器，而不是只停留在基础控件演示。
3. 在样式系统开始前，保持 Core / Reactive / Layout 三层 API 稳定，不再频繁改变组件输入模型。