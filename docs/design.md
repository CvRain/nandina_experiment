# Nandina 设计文档

## 1. 项目目标

Nandina 是一个现代 C++26 桌面 UI 框架实验项目，目标是在桌面端提供更接近前端框架的开发体验。

当前已经明确的设计原则：

1. 零宏。核心 API 均为纯 C++ 表达式，不依赖代码生成或预处理阶段扩展。
2. 组合优于继承。组件功能优先通过 Widget 组合、行为组合和响应式绑定扩展。
3. 响应式优先。状态变化应自然驱动 UI，而不是把刷新责任留给业务层手工同步。
4. 分层收敛。先稳定 Core、Reactive、Layout，再扩展 Theme、Text 与更完整的组件库。
5. 后端透明。渲染后端切换不应影响组件层 API。

补充背景可参考 [docs/meeting-notes-2026-04-10.md](docs/meeting-notes-2026-04-10.md)。

---

## 2. 技术选型

| 层 | 选型 | 说明 |
| :-- | :-- | :-- |
| 语言 | C++26 + Modules | 以模块为组织边界，减少头文件级联问题 |
| 窗口与事件 | SDL3 | 跨平台窗口系统与输入事件入口 |
| 矢量渲染 | ThorVG | 当前已用于软件后端绘制 |
| 文本 | FreeType + HarfBuzz | 作为后续完整文字栈目标 |
| 布局 | Row / Column / Stack | 当前自实现，后续保留接入 Yoga 的空间 |
| 构建 | CMake + Ninja + Presets | 适配 C++ Modules 与 vcpkg 工作流 |
| 包管理 | vcpkg manifest | 声明式依赖与可复现环境 |

渲染后端策略保持为：WebGPU / Vulkan 优先，其次 OpenGL，再次软件后端。当前仓库主要验证软件后端路径。

---

## 3. 当前架构

### 3.1 模块关系

```
Application → Window → Core ← Reactive
                    ↓
                  Layout
                    ↓
               Components / Pages
```

| 模块 | 职责 |
| :-- | :-- |
| `Nandina.Types` | 基础值类型，例如位置、尺寸、矩形 |
| `Nandina.Core` | Widget 树、事件分发、脏区传播、基础组件基类 |
| `Nandina.Reactive` | 状态、派生值、副作用、集合变更、属性输入模型 |
| `Nandina.Layout` | Row / Column / Stack 容器与基础布局能力 |
| `Nandina.Components.Label` | 当前已稳定的文本型基础组件 |
| `Nandina.Window` | SDL3 + ThorVG 窗口与渲染驱动 |

### 3.2 运行时数据流

```
SDL event
  ↓
Window translate / dispatch
  ↓
Widget hit test / handle_event
  ↓
State / StateList mutation
  ↓
Effect / direct subscriptions / list watchers
  ↓
mark_dirty() upward propagation
  ↓
render_widget() subtree pruning
  ↓
ThorVG canvas submit
```

---

## 4. 响应式层设计

### 4.1 两套互补模型

Nandina 当前明确保留两套互补的响应式模型，而不是试图把一切都折叠成单一抽象。

| 类型 | 用途 |
| :-- | :-- |
| `State<T>` | 单值状态、自动依赖追踪、`Computed` 与 `Effect` |
| `ReadState<T>` | `State<T>` 的只读视图，适合父传子 |
| `Prop<T>` | 统一组件属性入口，可承载静态值或只读响应式输入 |
| `StateList<T>` | 结构化集合状态，保留 `Inserted / Removed / Updated / Moved / Reset` 语义 |

设计边界如下：

- `State<T>` 用于标量状态、派生文本、选中值、局部可写状态。
- `StateList<T>` 用于列表、树、文件节点集合等结构化数据。
- 子组件默认消费 `ReadState<T>` 或 `Prop<T>`，父组件保留写权限。

### 4.2 `StateList<T>` 到自动依赖图的桥接

`StateList<T>` 不直接加入自动依赖图，但提供显式桥接接口：

- `version()`
- `tracked_size()`
- `tracked_empty()`
- `tracked_items()`

这使得列表既能保留精确 diff 语义，又能在需要时参与 `Computed` / `Effect` 的粗粒度派生。

### 4.3 最小批量更新

Reactive 当前已经实现最小 `batch(...)` 语义，用于把多个连续 `State::set()` 合并为一次观察者 flush：

- 批次内对同一 invalidator 去重。
- 外层批次结束时统一 flush。
- effect 在批次结束后读取最终一致状态。
- 不做回滚，不做跨线程事务上下文。
- 不尝试合并 `StateList<T>` 的 diff 事件。

### 4.4 生命周期与异常安全

- `Component` 内置 `EffectScope scope_`，组件析构时自动断开 effect。
- 依赖追踪上下文通过 RAII 守卫恢复，避免异常后 thread-local 状态污染。
- `State::notify()` 在观察者抛出异常时会恢复内部观察者状态，避免通知链损坏。
- `EventSignal::emit()` 会继续执行剩余回调，并在结束后重抛首个异常。

---

## 5. Core 与脏区传播

Widget 当前依赖两类标记进行最小重绘：

- `dirty_`：当前节点需要重绘。
- `has_dirty_child_`：子树内存在脏节点。

`mark_dirty()` 会向上冒泡，渲染阶段则通过 `!dirty && !has_dirty_child` 跳过整棵干净子树。这一机制已经与 `Effect` 驱动的 UI 更新结合运行。

---

## 6. 组件模型

继承链保持克制：

```
Widget
  └── Component
        └── CompositeComponent / Page / Label / Button ...
```

当前原则：

- 组件能力优先通过组合添加，而不是继续加深继承层次。
- 组件对外暴露 Props 或链式 API，但内部应尽量收敛到同一套状态输入模型。
- Button 目前仍偏向验证型基础组件，后续高级组件不应以复制 Button 内部实现为扩展方式。

---

## 7. 布局系统

当前已实现：

- `Row`
- `Column`
- `Stack`
- `gap()` / `padding()` / `align_items()` / `justify_content()` 等基础容器配置

当前定位：

- 足以支撑示例页面、基础组件组合与响应式验证。
- 仍需要更多真实页面场景来校验边界和 API 手感。
- Yoga 仍是后续候选后端，但不作为当前里程碑前提。

---

## 8. 示例页的角色

示例页不再只是原始控件堆叠，而应承担“验证当前基础层是否可用”的角色。

当前的 [example/pages/DemoPage.ixx](example/pages/DemoPage.ixx) 使用一个真实的“列表选择”场景来同时验证：

- `StateList<T>` 驱动列表内容
- `tracked_*` 生成摘要与派生文本
- `batch(...)` 合并按钮点击中的多次状态更新
- `ReadState<T>` / `Prop<T>` 驱动只读组件输入

---

## 9. 构建与验证

推荐本地流程：

1. `cmake --preset debug-vcpkg`
2. `cmake --build --preset debug-vcpkg`
3. `ctest --test-dir build/debug-vcpkg --output-on-failure`

如果需要更稳定的 modules 诊断，再额外运行 `debug-clang-vcpkg` 为 clangd 生成辅助构建信息。

---

## 10. 当前里程碑状态

| 里程碑 | 内容 | 状态 |
| :-- | :-- | :-- |
| M0 | 窗口、事件分发、基础渲染链路 | ✅ 完成 |
| M1 | Reactive、Label、dirty 剪枝、测试基线 | ✅ 完成 |
| M2 | Row / Column / Stack、示例页集成验证 | 🚧 进行中 |
| M3 | Theme / Token / Variant | ⬜ 未开始 |
| M4 | 完整文字栈 | ⬜ 未开始 |
| M5 | 更丰富组件库与 Yoga 接入 | ⬜ 未开始 |
