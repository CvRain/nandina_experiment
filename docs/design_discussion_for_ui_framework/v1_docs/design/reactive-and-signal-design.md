# Nandina 响应式系统与信号机制设计

> 文档状态：讨论稿  
> 更新日期：2026-04-08  
> 涉及模块：`Nandina.Reactive.Prop` · `Nandina.Reactive.StateList` · `Nandina.Core.Signal`

---

## 一、背景与设计目标

Nandina 的整体组件设计方向参考两个来源：

- **Angular 风格**：组件的定义范式——`Props` 结构体描述输入，组件类持有状态，`build()` 方法描述视图。
- **Flutter 风格**：单个组件的内部组织——Widget 树、不可变 Props、有状态的 State 与无状态 StatelessWidget 的对应。

核心问题是：在 C++ 中如何以"足够舒适"的方式统一这两者，同时减少开发者心智负担（参考 Kotlin Multiplatform 的设计哲学）。

---

## 二、模块归属与依赖图

```
src/reactive/
├── Reactive.ixx        Nandina.Reactive          ← umbrella（已有）
├── Prop.ixx            Nandina.Reactive.Prop      ← 新增
└── StateList.ixx       Nandina.Reactive.StateList ← 新增

src/core/
└── Signal.ixx          Nandina.Core.Signal        ← 升级现有文件
```

依赖方向（从底层到上层）：

```
std (atomic / functional / mutex)
    ↓
Nandina.Core.Signal          — 无状态的事件广播，不依赖响应式
    ↓
Nandina.Reactive             — State<T> / Computed<T> / Effect / EffectScope
    ↓
Nandina.Reactive.Prop        — Prop<T>，统一静态值与 State<T>
Nandina.Reactive.StateList   — StateList<T>，可观测集合
    ↓
Nandina.Core.Widget / Component  — 消费 Prop/StateList，订阅 Signal
```

---

## 三、`Prop<T>` 设计

### 3.1 动机

组件的属性来源有两种情况：
1. **静态值**：调用方直接传入一个字面量或普通对象，组件不需要追踪变化。
2. **响应式状态**：调用方传入一个 `State<T>`，组件需要在值变化时自动 rebuild。

`Prop<T>` 是对这两种情况的统一包装，让组件的接口不需要区分"我的调用方是否使用了响应式"。

### 3.2 接口设计

```cpp
// Nandina.Reactive.Prop
export module Nandina.Reactive.Prop;
import Nandina.Reactive;  // State<T>

export namespace Nandina {

template<typename T>
class Prop {
public:
    // 从静态值构造（隐式）
    Prop(T value);

    // 从 State<T> 构造（隐式）
    Prop(State<T>& state);

    // 读取当前值
    [[nodiscard]] auto get() const -> const T&;

    // 判断是否是响应式绑定
    [[nodiscard]] auto is_reactive() const -> bool;

    // 若是响应式，注册变化回调（供 Component 内部使用）
    // 返回 Connection，生命周期由持有者管理
    auto on_change(std::function<void(const T&)> callback) -> Connection;

private:
    std::variant<T, std::reference_wrapper<State<T>>> value_;
};

} // namespace Nandina
```

### 3.3 使用范式（对应 Angular Props 风格）

```cpp
struct ButtonProps {
    Prop<std::string>          text    = "Click me";
    Prop<Style>                style   = Style{};
    std::function<void()>      on_click;
};

class MyButton : public CompositeComponent<ButtonProps> {
public:
    auto build() -> WidgetPtr override {
        return Button(props().text.get())
            .style(props().style.get())
            .on_click(props().on_click)
            .build();
    }
};
```

---

## 四、`StateList<T>` 设计

### 4.1 动机

`State<T>` 处理的是单个可观测值。但 UI 中大量场景是**可观测的集合**：列表渲染、选项卡、路由历史……  
直接用 `State<std::vector<T>>` 有两个问题：
- 每次修改（push/pop/sort）都替换整个 vector，通知粒度太粗。
- 无法区分"哪条数据变了"，无法实现高效的差量更新（diff/patch）。

`StateList<T>` 解决这个问题，提供操作级别的细粒度通知。

### 4.2 变更事件类型

```cpp
enum class ListChangeKind {
    Inserted,   // 新增了一项
    Removed,    // 删除了一项
    Updated,    // 某项的值被替换
    Moved,      // 某项被移动到新位置（用于 sort/reorder）
    Reset,      // 整个列表被替换（assignAll）
};

struct ListChange<T> {
    ListChangeKind kind;
    std::size_t    index;
    std::optional<T> old_value;   // Removed / Updated / Moved 时有效
    std::optional<T> new_value;   // Inserted / Updated / Moved 时有效
};
```

### 4.3 接口设计

```cpp
export module Nandina.Reactive.StateList;
import Nandina.Reactive;

export namespace Nandina {

template<typename T>
class StateList {
public:
    // 只读访问
    [[nodiscard]] auto size()  const -> std::size_t;
    [[nodiscard]] auto empty() const -> bool;
    [[nodiscard]] auto at(std::size_t i) const -> const T&;
    [[nodiscard]] auto operator[](std::size_t i) const -> const T&;

    // 写操作（每次操作后 emit 对应 ListChange）
    auto push_back(T value)                          -> void;
    auto insert(std::size_t index, T value)          -> void;
    auto remove_at(std::size_t index)                -> void;
    auto update_at(std::size_t index, T new_value)   -> void;
    auto assign_all(std::vector<T> new_data)         -> void;
    auto clear()                                     -> void;

    // 订阅变更通知
    auto on_change(std::function<void(const ListChange<T>&)> cb) -> Connection;

    // 订阅任意变化（粗粒度，等同于 State<T> 的 watch）
    auto watch(std::function<void()> cb) -> Connection;

    // 遍历（只读）
    auto begin() const;
    auto end()   const;

private:
    std::vector<T>                              data_;
    EventSignal<ListChange<T>>                  on_change_;
};

} // namespace Nandina
```

### 4.4 与 Layout 的配合

Layout 的列表类组件（如 `Column`、`Row`）最终目标是接受 `StateList<WidgetPtr>`，实现细粒度的 diff 更新——只有插入/删除对应的子 Widget，不需要整体重建。

---

## 五、`Signal` 升级设计（`Nandina.Core.Signal`）

### 5.1 现状

当前 `Signal.ixx` 已实现 `Connection` + `EventSignal<Args...>`，功能基本完整，但存在两个问题：
1. 非线程安全（多线程 emit 与 disconnect 并发时存在 UB）。
2. 没有"一次性连接"（`connect_once`）支持。

### 5.2 升级目标

| 特性 | 现状 | 目标 |
|---|---|---|
| 线程安全 | ✗ | ✓ `shared_mutex` 保护 slot 列表 |
| `connect_once` | ✗ | ✓ 触发一次后自动断开 |
| `Connection` 析构自动断开 | ✗（需手动 disconnect） | 可选：`ScopedConnection` RAII wrapper |
| emit 期间 disconnect 安全 | ✓（标记非活跃，延迟清理） | 保持，额外处理重入 |

### 5.3 新增接口

```cpp
// 一次性连接
auto connect_once(Slot slot) -> Connection;

// RAII 自动断开（ScopedConnection 析构时调用 disconnect）
class ScopedConnection {
public:
    explicit ScopedConnection(Connection conn);
    ~ScopedConnection();  // 自动 disconnect
    ScopedConnection(const ScopedConnection&) = delete;
    ScopedConnection(ScopedConnection&&) noexcept = default;
};
```

### 5.4 与 `Nandina.Reactive` 的关系

`EventSignal` 是**事件广播**，用于 UI 事件（click、keydown 等），不参与响应式数据流。  
`State<T>` 是**数据状态**，用于驱动 rebuild。  
两者不合并，各司其职，但 `Component` 层可以同时持有两者。

---

## 六、组件范式总结

基于以上设计，一个完整的 Nandina 组件写法如下：

```cpp
// Props：描述外部输入，支持静态值与响应式值
struct CounterProps {
    Prop<int>          initial_value = 0;
    Prop<std::string>  label         = "Counter";
    std::function<void(int)> on_change;   // 纯事件，不需要 Prop 包装
};

// 组件：持有内部状态，build() 描述视图
class Counter : public CompositeComponent<CounterProps> {
    State<int> count_{ props().initial_value.get() };

    auto on_init() -> void override {
        // 若 Props 的 initial_value 是响应式的，同步变化
        props().initial_value.on_change([this](int v) {
            count_.set(v);
        });
    }

public:
    auto build() -> WidgetPtr override {
        return Column()
            .child(Label(props().label.get() + ": " + std::to_string(count_.get())))
            .child(
                Row()
                .child(Button("-").on_click([this]{ 
                    count_.set(count_.get() - 1); 
                    if (props().on_change) props().on_change(count_.get());
                }))
                .child(Button("+").on_click([this]{ 
                    count_.set(count_.get() + 1);
                    if (props().on_change) props().on_change(count_.get());
                }))
            )
            .build();
    }
};
```

---

## 七、CMakeLists.txt 变更

在 `MODULE_SOURCES` 的 `reactive` 区域追加：

```cmake
src/reactive/Prop.ixx
src/reactive/StateList.ixx
```

`Reactive.ixx` umbrella 补充导出：

```cpp
export import Nandina.Reactive.Prop;
export import Nandina.Reactive.StateList;
```

---

## 八、后续待讨论

- [ ] `Layout` 的抽象层级提升：目前 `Layout.ixx` 封装程度不够，使用不够自然，需要重新设计接口。
- [ ] `Page` 与 `Router` 的关系确认：`Page` 是否继承 `CompositeComponent`，还是独立层级？
- [ ] Widget 树的 diff 算法：`StateList<WidgetPtr>` 驱动的细粒度更新，如何映射到 ThorVG/SDL 的绘制调用。
- [ ] `Prop<T>` 的双向绑定扩展：是否需要 `v-model` 类似的 `BindProp<T>` 支持写回。