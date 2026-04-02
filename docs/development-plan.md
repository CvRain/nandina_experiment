# Nandina 开发计划（M1 – M5）

## M1：响应式核心 + Label + dirty 剪枝

### 1.1 Nandina.Reactive 模块

- [x] 创建 `src/reactive/Reactive.ixx`，导出 `Nandina.Reactive` 模块
- [x] 实现 `State<T>`：`operator()()` 读值并自动注册依赖，`set()` 写值并通知观察者
- [x] 实现 `ReadState<T>`：只读视图，由 `State<T>::as_read_only()` 创建
- [x] 实现 `Computed<F>`：惰性派生状态，自动追踪 State 依赖
- [x] 实现 `Effect`：副作用函数，依赖变化时自动重新执行
- [x] 实现 `EffectScope`：RAII 生命周期容器，析构时断开所有 Effect
- [x] `detail::current_invalidator` thread_local 依赖追踪机制

### 1.2 Widget dirty 剪枝增强

- [x] `Widget` 添加 `parent_` 非拥有指针（弱引用）
- [x] `Widget` 添加 `has_dirty_child_` 标志位
- [x] `add_child()` 设置 `child->parent_ = this`
- [x] `mark_dirty()` 向上冒泡 `bubble_dirty_child()`
- [x] `render_widget()` 剪枝：`!dirty && !has_dirty_child` 时跳过子树
- [x] `clear_dirty()` 同时清除 `dirty_` 和 `has_dirty_child_`
- [x] `for_each_child()` 改为 public

### 1.3 Component EffectScope

- [x] `Component` 基类添加 `EffectScope scope_` 成员
- [x] Core.ixx 引入 `import Nandina.Reactive`

### 1.4 Label 组件

- [x] 创建 `src/components/Label.ixx`，导出 `Nandina.Components.Label`
- [x] `LabelProps` 结构体：`text_signal` 可选响应式绑定
- [x] `Label::Create()` 无参工厂
- [x] `Label::Create(LabelProps)` Props 工厂，支持响应式文本绑定
- [x] 链式 API：`text()`, `font_size()`, `text_color()`
- [x] 私有成员：`State<std::string> text_`，`float font_size_`，`text_r/g/b/a_`
- [x] 私有构造器添加 Effect：读取 `text_()` → 调用 `mark_dirty()`

---

## M2：布局系统

### 2.1 LayoutContainer 基类

- [x] 创建 `src/layout/Layout.ixx`，导出 `Nandina.Layout`
- [x] `Align` 枚举：start / center / end / stretch / space_between / space_around
- [x] `LayoutContainer` 抽象类（继承 Component）
- [x] 链式 API：`gap()`, `padding()`, `align_items()`, `justify_content()`
- [x] `add(std::unique_ptr<Widget>)` 返回 `LayoutContainer&`
- [x] 纯虚 `layout()` 方法

### 2.2 具体布局容器

- [x] `Column::Create()`，`layout()` 纵向排列子组件
- [x] `Row::Create()`，`layout()` 横向排列子组件
- [x] `Stack::Create()`，`layout()` 叠放子组件

### 2.3 集成测试

- [ ] 使用 Row/Column 替换 main.cpp 中的绝对坐标布局
- [ ] 验证 gap/padding 属性生效

---

## M3：样式系统

### 3.1 ThemeTokens

- [ ] 设计 `ThemeTokens` 结构体（颜色、圆角、间距、字体 Token）
- [ ] 实现默认 Light/Dark 主题
- [ ] 组件可读取全局主题 Token

### 3.2 Variant Recipe

- [ ] Button 支持 Primary / Ghost / Destructive Variant
- [ ] 各 Variant 自动解析对应绘制参数
- [ ] 状态样式（hover / pressed / focused / disabled）

---

## M4：文字渲染

### 4.1 文字整形与光栅化

- [ ] 集成 HarfBuzz 进行 UTF-8 文本整形，输出 GlyphRun
- [ ] 集成 FreeType 进行字形光栅化
- [ ] 实现 Glyph Atlas 缓存（纹理图集）

### 4.2 ThorVG 集成

- [ ] 将 Glyph Atlas 以 ThorVG 纹理形式输出到画布
- [ ] Label 组件实现实际文字渲染（当前为骨架代码）

---

## M5：组件库 + Yoga

### 5.1 组件库扩展

- [ ] `Input` 组件（文本输入框）
- [ ] `Checkbox` 组件
- [ ] `Switch` 组件
- [ ] `Dialog` 组件
- [ ] `Dropdown` 组件
- [ ] `Toast` 通知组件

### 5.2 Yoga 接入

- [ ] 集成 Yoga 布局引擎
- [ ] Row/Column/Stack 内部计算替换为 Yoga
- [ ] 对外 API 保持不变

---

## 当前执行建议

M1 → M2 → M3 → M4 → M5 顺序推进，每个里程碑完成后在 `docs/design.md` 里程碑表中更新状态。
