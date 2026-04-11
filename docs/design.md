# Nandina 设计文档

## 1. 项目目标

Nandina 目标是在现代 C++ 桌面端提供接近前端框架的开发体验：

1. **零宏**：完全不依赖宏魔法，所有 API 均为纯 C++ 表达式。
2. **现代 C++26**：使用 Modules、概念、范围等最新语言特性。
3. **组合优于继承**：复杂组件由 Action 单元组合而成，不形成继承链臃肿体。
4. **响应式状态**：`State<T>` / `Computed<F>` / `Effect` 驱动 UI 自动更新，无需手动 `setState`。
5. **跨平台渲染**：WebGPU(Vulkan/Metal/DX12) > OpenGL > 软件光栅化，后端透明于组件代码。

---

## 2. 技术选型

| 层 | 选型 | 说明 |
| :-- | :-- | :-- |
| 语言 | C++26 + Modules | 零头文件，概念约束，最新标准 |
| 窗口/事件 | SDL3 | 跨平台窗口与事件，轻量稳定 |
| 矢量渲染 | ThorVG | 软件 / OpenGL / WebGPU 后端，路径+SVG 能力 |
| 文字 | FreeType + HarfBuzz | Unicode 字形光栅化 + 文本整形 |
| 布局 | Row/Column/Stack → Yoga（后期） | 先自实现 Flex 容器，后期接入 Yoga |
| 构建 | CMake + Ninja | C++26 Modules 依赖扫描，CMakePresets |
| 包管理 | vcpkg manifest 模式 | 声明式依赖，CI 可复现 |

### 渲染后端优先级策略

```
WebGPU (Vulkan / Metal / DX12)
        ↓ 探测失败
    OpenGL
        ↓ 探测失败
  SwCanvas（软件光栅化）
```

后端选择对组件代码完全透明，ThorVG 统一抽象渲染面。

---

## 3. 架构总览

### 3.1 分层架构图

```
Application → Window → Layout → Core ← Reactive → Style → Text → Types
```

| 模块 | 职责 |
| :-- | :-- |
| `Nandina.Types` | Color、Position、Size、Rect 等基础值类型 |
| `Nandina.Core` | Widget 树 / 几何 / dirty 标记 / 事件分发 / Component / Button |
| `Nandina.Reactive` | State / ReadState / Computed / Effect / EffectScope |
| `Nandina.Style` | ThemeTokens、Design Token、Variant Recipe |
| `Nandina.Text` | UTF-8 整形 → GlyphRun → Glyph Atlas → ThorVG |
| `Nandina.Layout` | Row / Column / Stack，后期接入 Yoga |
| `Nandina.Components.*` | Label / Button / Input 等高层组件 |
| `Nandina.Window` | NanWindow（SDL3 + ThorVG）、WindowController |
| `Nandina.Application` | 应用入口，初始化 SDL3 / ThorVG |

### 3.2 数据流

```
SDL 事件
  ↓
NanWindow::process_events()
  ↓
translate_and_dispatch() → Nandina::Event
  ↓
Widget 树命中测试 (dispatch_event)
  ↓
handle_event() → 修改 State<T>
  ↓
Effect 触发 → mark_dirty() 向上冒泡
  ↓
render_widget() 剪枝（!dirty && !has_dirty_child 则跳过）
  ↓
ThorVG canvas submit → SDL 纹理更新
```

---

## 4. Nandina.Reactive 模块设计

### 4.1 核心类型

| 类型 | 说明 |
| :-- | :-- |
| `State<T>` | 可读写响应式值，`operator()()` 读值并自动注册依赖，`set()` 写值并通知观察者 |
| `ReadState<T>` | 只读视图，由 `State<T>::as_read_only()` 创建，传给子组件 Props |
| `Computed<F>` | 惰性派生状态，自动追踪 State 依赖，访问时按需重算 |
| `Effect` | 副作用函数，每次依赖 State 发生变化时重新执行 |
| `EffectScope` | RAII 生命周期容器，scope 析构时所有 Effect 自动断开 |

### 4.2 依赖追踪内部机制

```cpp
// thread_local 当前 invalidator 指针
inline thread_local std::function<void()>* current_invalidator = nullptr;

// State::operator()() 调用时
void track_access() const {
    if (current_invalidator) {
        observers_.push_back({ next_id_++, true, *current_invalidator });
    }
}

// Effect::run() 执行时
void run() {
    self_invalidator_ = [this]{ run(); };     // 重新执行的闭包
    auto* prev = current_invalidator;
    current_invalidator = &self_invalidator_;
    fn_();                                     // 执行副作用，期间读取的 State 会注册 self_invalidator_
    current_invalidator = prev;
}
```

### 4.3 作用域层级

| 层级 | 所有者 | 说明 |
| :-- | :-- | :-- |
| 组件局部 | Component::scope_ | 随组件析构自动清理 |
| 父→子传递 | Props::Prop<T> / ReadState<T> | 父组件持有 State，子组件通过只读输入模型消费，Widget 树保证子不长于父 |
| 全局应用 | AppStore | 跨组件共享状态，手动管理生命周期 |

---

## 5. dirty 标记设计

Widget 持有两个标志位：

- `dirty_`：本节点需要重绘。
- `has_dirty_child_`：子树中存在 dirty 节点。

### 5.1 mark_dirty() 向上冒泡

```
widget.mark_dirty()
  → dirty_ = true
  → parent_->bubble_dirty_child()
       → parent_->has_dirty_child_ = true
       → parent_->parent_->bubble_dirty_child() ...
```

### 5.2 render_widget() 剪枝

```
render_widget(widget)
  if (!widget.is_dirty() && !widget.has_dirty_child())
      return;   // 跳过整棵子树
  if (widget.is_dirty())
      // 重绘本节点
  for_each_child → render_widget(child)
```

---

## 6. 组件模型

### 6.1 继承关系

```
Widget  (树 / 几何 / dirty / 事件)
  └── Component  (业务基类 + EffectScope scope_)
        └── Label / Button / Input ...
```

### 6.2 Action 组合模型

行为单元（Action）以组合而非继承方式附加到组件：

| Action | 职责 |
| :-- | :-- |
| `HoverAction` | 悬停状态追踪 |
| `PressAction` | 按压状态追踪 |
| `FocusAction` | 焦点状态追踪 |

---

## 7. 布局系统

| 容器 | 排布方式 |
| :-- | :-- |
| `Row` | 子组件横向排列，支持 gap / padding / align_items / justify_content |
| `Column` | 子组件纵向排列 |
| `Stack` | 子组件叠放于同一位置 |

后期将 Row/Column 内部计算替换为 Yoga，对外 API 不变。

### 布局属性

```
gap           // 子间距
padding       // 内边距（统一 或 水平/垂直 分开设置）
align_items   // 交叉轴对齐
justify_content // 主轴对齐
```

---

## 8. 文字渲染流程

```
UTF-8 字符串
  ↓ HarfBuzz 整形
GlyphRun（字形序列 + 位置）
  ↓ FreeType 光栅化
Glyph Atlas 缓存（纹理）
  ↓ ThorVG 纹理贴图
屏幕输出
```

---

## 9. Style / Theme

### ThemeTokens 结构

| 分类 | Token |
| :-- | :-- |
| 颜色 | background, foreground, primary, primary_foreground, secondary, destructive, muted, border, ring |
| 圆角 | radius_sm, radius_md, radius_lg |
| 间距 | spacing_1 ~ spacing_4 |
| 字体 | font_size_sm, font_size_base, font_size_lg |

---

## 10. 组件定义范式

Props 结构体 + 链式 API 双轨并行：

```cpp
// 链式调用风格
auto label = Label::Create()
    .text("Hello")
    .font_size(16);

// Props 结构体风格（适合响应式绑定）
State<std::string> name{"Nandina"};
auto label = Label::Create({ .text = Prop<std::string>{name.as_read_only()} });
```

组件输入边界约定：

- 父组件向子组件传递响应式数据时，优先传 `ReadState<T>` 或 `Prop<T>`，避免把可写 `State<T>` 直接暴露给子组件。
- `State<T>` 主要用于组件内部的本地可写状态，或页面/容器对共享状态的拥有端。
- `Prop<T>` 统一组件属性入口，既可表达静态值，也可表达基于 `ReadState<T>` 的只读响应式输入。

---

## 11. 构建系统

### CMakePresets

| Preset | 说明 |
| :-- | :-- |
| debug | Debug + ASan/UBSan |
| release | Release -O3 |
| debug-vcpkg | Debug + vcpkg manifest |
| release-vcpkg | Release + vcpkg manifest |
| ci | CI 构建（继承 release-vcpkg） |

### vcpkg manifest 依赖

sdl3 / thorvg / freetype / harfbuzz / yoga

---

## 12. 里程碑

| 里程碑 | 内容 | 状态 |
| :-- | :-- | :-- |
| M0 | 窗口 + Button 组合 + 事件分发 + ThorVG SwCanvas | ✅ 完成 |
| M1 | Reactive 模块（State/Computed/Effect/EffectScope）+ Label + dirty 剪枝 | ✅ 完成 |
| M2 | Row / Column / Stack 布局容器 | ⬜ 待开始 |
| M3 | ThemeTokens + Design Token + Variant Recipe | ⬜ 待开始 |
| M4 | FreeType + HarfBuzz 文字渲染 + Glyph Atlas | ⬜ 待开始 |
| M5 | 组件库扩展（Input/Checkbox/Switch 等）+ Yoga 接入 | ⬜ 待开始 |
