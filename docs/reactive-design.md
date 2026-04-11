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
    Nandina::Prop<int> count;
};

// 父组件构造只读输入
CounterProps props{
    .count = Nandina::Prop<int>{ count.as_read_only() }
};

// 子组件统一读取输入
int val = props.count.get();
```

如果组件需要单独声明只读绑定接口，也应优先接收 `ReadState<T>`：

```cpp
auto label = Nandina::Label::Create();
label->bind_text(count.as_read_only(), [](int value) {
    return std::format("{}", value);
});
```

推荐边界：

- 父组件拥有 `State<T>`。
- 子组件通过 `ReadState<T>` 或 `Prop<T>` 消费响应式输入。
- 只有组件内部本地状态才直接使用可写 `State<T>`。

```cpp
// ReadState 直接读取时也会自动注册依赖
int val = read_count();   // 读取并自动注册依赖
```

**安全保证**：Widget 树结构保证子组件不会比父组件存活更长时间，因此 `ReadState<T>` 持有的裸指针在子组件生命周期内始终有效。

---

## 4. State<T> 与 StateList<T> 的角色边界

`State<T>` 和 `StateList<T>` 名字相近，但它们并不是同一套机制的两个外形。

更准确地说，它们代表两种不同的响应式模型：

### `State<T>`：值语义、自动依赖追踪

`State<T>` 适合表达单值状态、派生值和自动驱动的 UI 绑定，例如：

- 当前路径
- 搜索关键字
- 是否显示隐藏文件
- 当前选中节点 id
- 标题栏、状态栏、面包屑文本

这类状态的特点是：

- 读取时自动建立依赖
- 写入时自动触发相关 `Effect` / `Computed`
- 使用体验接近 Angular Signals / SolidJS signals

```cpp
State<std::string> current_path{"/home/user"};
State<std::string> search_text{""};

Computed header_text{[&] {
    return std::format("{} ({})", current_path(), search_text());
}};

Effect refresh_header{[&] {
    title_label->text(header_text());
}};
```

### `StateList<T>`：结构语义、显式变更流

`StateList<T>` 适合表达列表、树、文件节点集合这类结构化数据，例如：

- 当前目录文件列表
- 树节点 children
- 最近打开文件列表
- 标签页集合

这类状态的特点是：

- 不只是关心“值变了”，而是关心“哪里插入、哪里删除、哪里移动、哪里更新”
- 更适合精准的节点挂载、局部复用和顺序调整
- 与 `Effect` 的自动依赖图分离，避免把所有集合变更都退化成整个派生逻辑重跑

```cpp
StateList<FileNode> entries;

auto conn = entries.on_change([&](const ListChange<FileNode>& change) {
    switch (change.kind) {
    case ListChangeKind::Inserted:
        list_view->insert_row(change.index, make_file_row(*change.value));
        break;
    case ListChangeKind::Removed:
        list_view->remove_row(change.index);
        break;
    case ListChangeKind::Updated:
        list_view->update_row(change.index, *change.value);
        break;
    case ListChangeKind::Moved:
        list_view->move_row(*change.old_index, change.index);
        break;
    case ListChangeKind::Reset:
        list_view->rebuild_all(entries.items());
        break;
    }
});
```

这不是缺陷，而是有意识的架构分工。

如果未来要做文件管理器、树控件、虚拟列表、懒加载目录树，这种结构语义会非常重要。

---

## 5. `tracked_*`：StateList 到 Effect 图的显式桥接

虽然 `StateList<T>` 不直接参与自动依赖追踪，但有些场景只需要集合的“粗粒度派生值”，例如：

- 当前列表数量
- 列表是否为空
- 过滤后的结果数量
- 基于整个列表生成摘要文本

为此，`StateList<T>` 当前提供了一组显式桥接 API：

- `version()`
- `tracked_size()`
- `tracked_empty()`
- `tracked_items()`

这组 API 的定位不是把 `StateList<T>` 重新改造成 `State<std::vector<T>>`，而是明确提供一座桥，让集合状态在“需要时”回到 `Computed` / `Effect` 世界。

```cpp
State<std::string> search_text{""};
StateList<FileNode> entries;

Computed filtered_count{[&] {
    auto keyword = search_text();
    const auto& items = entries.tracked_items();

    return std::ranges::count_if(items, [&](const FileNode& node) {
        return keyword.empty() || node.name.contains(keyword);
    });
}};

Effect refresh_summary{[&] {
    summary_label->text(std::format("Matched: {}", filtered_count()));
}};
```

推荐使用规则：

1. 需要精确结构更新时，用 `on_change()` / `watch()`。
2. 只需要粗粒度派生值时，用 `tracked_*`。
3. 不要把 `tracked_*` 当成替代结构化 diff 的主路径。

这样可以同时保留：

- Angular/Solid 风格的自动值响应式体验
- 面向树和文件管理器的高精度结构更新能力

---

## 6. 文件管理器 / 树结构的组合方式（伪代码）

下面的伪代码展示两套模型如何协作，而不是互相替代。

```cpp
struct FileNode {
    std::string id;
    std::string name;
    bool is_directory = false;
};

class FileExplorerPage : public Component {
private:
    State<std::string> current_path{"/home/user"};
    State<std::string> search_text{""};
    State<std::string> selected_id{""};
    StateList<FileNode> visible_entries;

public:
    void setup() {
        scope_.add([this] {
            breadcrumb_label_->text(current_path());
        });

        scope_.add([this] {
            const auto& items = visible_entries.tracked_items();
            auto keyword = search_text();

            auto matches = std::ranges::count_if(items, [&](const FileNode& node) {
                return keyword.empty() || node.name.contains(keyword);
            });

            summary_label_->text(std::format("Matched: {}", matches));
        });

        list_conn_ = visible_entries.on_change([this](const ListChange<FileNode>& ch) {
            switch (ch.kind) {
            case ListChangeKind::Inserted:
                list_view_->insert_row(ch.index, make_row(*ch.value));
                break;
            case ListChangeKind::Removed:
                list_view_->remove_row(ch.index);
                break;
            case ListChangeKind::Updated:
                list_view_->update_row(ch.index, *ch.value);
                break;
            case ListChangeKind::Moved:
                list_view_->move_row(*ch.old_index, ch.index);
                break;
            case ListChangeKind::Reset:
                list_view_->rebuild_all(visible_entries.items());
                break;
            }
        });
    }
};
```

在这个模式里：

- `current_path` / `search_text` / `selected_id` 是值状态，用 `State<T>`。
- `visible_entries` 是结构状态，用 `StateList<T>`。
- 摘要栏之类的粗粒度派生用 `tracked_items()`。
- 真实列表行的增删改移用 `on_change()`。

---

## 7. Computed\<F\> 用法

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

## 8. Effect + EffectScope 用法

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

## 9. 依赖追踪内部机制

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

## 10. 最小事务式更新（提案）

这一节描述的是推荐中的未来能力，不表示当前源码已经实现。

### 设计动机

如果一个用户操作需要连续更新多个 `State<T>`：

```cpp
selected_id.set("file-a");
preview_name.set("file-a");
preview_loading.set(true);
status_text.set("Loading...");
```

那么依赖这些状态的 `Effect` 可能被重复触发多次，并且在执行过程中短暂看到不一致的中间状态。

这在 UI 层会表现为：

- 多余的重算
- 多余的 `mark_dirty()`
- 组件短暂读到“改了一半”的状态组合

### 推荐的最小形态

建议未来先引入一个非常克制的批量提交能力：

```cpp
batch([&] {
    selected_id.set("file-a");
    preview_name.set("file-a");
    preview_loading.set(true);
    status_text.set("Loading...");
});
```

推荐语义：

1. `batch` 期间允许多个 `State::set()` 连续发生。
2. 每次 `set()` 只标记依赖 dirty，不立即重跑 effect。
3. `batch` 结束后统一 flush。
4. 同一个 `Effect` 在同一批次中至多重跑一次。

这更像 UI 提交阶段的批量刷新，不是完整数据库事务：

- 不要求 rollback
- 不要求隔离级别
- 不要求跨线程事务上下文

### 文件管理器场景伪代码

```cpp
batch([&] {
    current_path.set("/docs");
    selected_id.set("report-final.txt");
    breadcrumb_text.set("/docs/report-final.txt");
    preview_visible.set(true);
    status_text.set("Loading preview...");
});
```

这里的收益是：依赖这些状态的标题栏、预览区、状态栏 effect 会在批次结束后看到一份一致的最终状态。

### 对 v1 的建议边界

如果要在 v1 引入，只建议做最小版：

- 支持 `State<T>` 的批量 observer flush
- 支持 effect 去重
- 暂不做 `StateList<T>` 多次变更的 diff 合并

因为一旦要让 `StateList<T>` 也具备“事务内合并 diff”语义，复杂度会迅速上升到列表 reconciler 层级，不适合在响应式 v1 里一起吃下。

---

## 11. 生命周期规则

| 场景 | 规则 |
| :-- | :-- |
| Component 内的 Effect | 添加到 `scope_`，组件析构时自动断开 |
| 传给子组件的 ReadState / Prop | 子组件通过只读输入模型拿到响应式视图，Widget 树保证父先于子析构 |
| AppStore 全局 State | 手动管理 EffectScope，或使用单例生命周期 |
| Effect 移动后 | 当前实现中禁用移动，避免重定位导致的观察者失活 |
| Computed 移动后 | 当前实现中禁用移动，避免缓存失效回调悬垂 |

---

## 12. 与 Qt SIGNAL/SLOT 对比

| 特性 | Qt SIGNAL/SLOT | Nandina.Reactive |
| :-- | :-- | :-- |
| 语法 | `Q_OBJECT` 宏 + `moc` 预处理 | 纯 C++ 模板，零宏 |
| 依赖追踪 | 手动 `connect()` | 自动（访问 State 时注册） |
| 生命周期管理 | 手动 `disconnect()` 或 QObject 树 | RAII EffectScope 自动清理 |
| 类型安全 | 运行时字符串匹配（旧式）/ 函数指针 | 编译期模板类型检查 |
| 跨线程 | Qt::QueuedConnection 自动队列 | 目前单线程，thread_local 追踪 |
| 继承要求 | 必须继承 QObject | 任意类型均可持有 State |
