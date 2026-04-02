# Nandina.Reactive 响应式模块设计文档

## 1. 概述与设计目标

Nandina.Reactive 是 Nandina GUI 框架的响应式状态管理模块，目标：

1. **零宏**：纯 C++ 模板实现，无预处理器魔法。
2. **自动依赖追踪**：读取 `State<T>` 时自动注册当前 Effect 为观察者，无需手动订阅。
3. **RAII 生命周期**：`EffectScope` 析构时自动断开所有 Effect，杜绝悬垂观察者。
4. **组合友好**：与 Widget 树生命周期对齐，Component 内置 `EffectScope scope_`。

---

## 2. State\<T\> 用法

`State<T>` 是可读写的响应式值容器：

```cpp
Nandina::State<int> count{0};

// 读取当前值（在 Effect 内读取时自动注册依赖）
int current = count();   // 或 count.get()

// 写入新值（若值变化则通知所有观察者）
count.set(count() + 1);
```

### 获取只读视图

```cpp
// 传给子组件时，只暴露只读视图
Nandina::ReadState<int> read_count = count.as_read_only();
```

---

## 3. ReadState\<T\> 与父→子传递模式

`ReadState<T>` 是 `State<T>` 的非拥有只读视图，适合通过 Props 传递给子组件：

```cpp
// 父组件
struct CounterProps {
    const Nandina::State<int>* count_signal = nullptr;
};

// 子组件接收只读视图
auto read_count = props.count_signal->as_read_only();
int val = read_count();   // 读取并自动注册依赖
```

**安全保证**：Widget 树结构保证子组件不会比父组件存活更长时间，因此 `ReadState<T>` 持有的裸指针在子组件生命周期内始终有效。

---

## 4. Computed\<F\> 用法

`Computed<F>` 是惰性派生状态，第一次访问（或依赖变化后访问）时重新计算：

```cpp
Nandina::State<int> a{2};
Nandina::State<int> b{3};

Nandina::Computed sum{[&]{ return a() + b(); }};

int result = sum();   // 计算并缓存结果 → 5
a.set(10);
result = sum();       // 依赖变化，重新计算 → 13
```

---

## 5. Effect + EffectScope 用法

### Effect

`Effect` 在构造时立即执行一次副作用函数，并在依赖变化时自动重新执行：

```cpp
Nandina::State<std::string> text{"Hello"};

Nandina::Effect log_effect{[&]{
    std::println("text changed: {}", text());
}};
// 立即输出: text changed: Hello

text.set("World");
// 自动输出: text changed: World
```

### EffectScope

`EffectScope` 批量管理多个 Effect，析构时一次性断开所有观察者：

```cpp
{
    Nandina::EffectScope scope;
    scope.add([&]{ std::println("A: {}", stateA()); });
    scope.add([&]{ std::println("B: {}", stateB()); });
    // scope 析构 → 所有 Effect 自动断开
}
```

### Component 内置 scope_

`Component` 基类持有 `EffectScope scope_`，所有在组件内注册的 Effect 随组件析构自动清理：

```cpp
class MyLabel : public Nandina::Component {
    MyLabel() {
        scope_.add([this]{ mark_dirty(); });  // text_ 变化时触发重绘
    }
    Nandina::State<std::string> text_{"Label"};
};
```

---

## 6. 依赖追踪内部机制

### thread_local current_invalidator

```cpp
namespace Nandina::detail {
    inline thread_local std::function<void()>* current_invalidator = nullptr;
}
```

### State::track_access()

```cpp
void track_access() const {
    if (detail::current_invalidator) {
        observers_.push_back({
            next_id_++,
            true,
            *detail::current_invalidator   // 拷贝当前 invalidator
        });
    }
}
```

### Effect::run() 设置/恢复 current_invalidator

```cpp
void run() {
    if (!active_) return;
    self_invalidator_ = [this]{ run(); };          // 自身重新执行闭包
    auto* prev = detail::current_invalidator;
    detail::current_invalidator = &self_invalidator_;
    fn_();                                          // 执行副作用，读取的 State 会注册 self_invalidator_
    detail::current_invalidator = prev;             // 恢复上层 invalidator
}
```

**执行序列**：
1. `Effect::run()` 设置 `current_invalidator = &self_invalidator_`
2. `fn_()` 内部调用 `state()` → `State::track_access()` 把 `self_invalidator_` 注册为观察者
3. `fn_()` 返回后恢复 `current_invalidator`
4. `state.set(new_val)` → `State::notify()` → 调用 `self_invalidator_()` → `Effect::run()` 重新执行

---

## 7. 生命周期规则

| 场景 | 规则 |
| :-- | :-- |
| Component 内的 Effect | 添加到 `scope_`，组件析构时自动断开 |
| 传给子组件的 ReadState | 子组件通过 Props 拿到只读视图，Widget 树保证父先于子析构 |
| AppStore 全局 State | 手动管理 EffectScope，或使用单例生命周期 |
| Effect 移动后 | 移动构造正确转移 fn_ 和 active_，原对象 active_=false 不再触发 |
| Computed 析构后 | 依赖链中的 stale_ 标志不再更新，访问会重算但不引发 UB |

---

## 8. 与 Qt SIGNAL/SLOT 对比

| 特性 | Qt SIGNAL/SLOT | Nandina.Reactive |
| :-- | :-- | :-- |
| 语法 | `Q_OBJECT` 宏 + `moc` 预处理 | 纯 C++ 模板，零宏 |
| 依赖追踪 | 手动 `connect()` | 自动（访问 State 时注册） |
| 生命周期管理 | 手动 `disconnect()` 或 QObject 树 | RAII EffectScope 自动清理 |
| 类型安全 | 运行时字符串匹配（旧式）/ 函数指针 | 编译期模板类型检查 |
| 跨线程 | Qt::QueuedConnection 自动队列 | 目前单线程，thread_local 追踪 |
| 继承要求 | 必须继承 QObject | 任意类型均可持有 State |
