# Nandina 开发计划

本文件只保留当前仍有执行意义的计划项。历史问题分析与讨论过程，分别见 `report-1.txt` 与 [docs/meeting-notes-2026-04-10.md](docs/meeting-notes-2026-04-10.md)。

---

## M1：响应式核心与基础组件

状态：已完成

### 已完成

- [x] 单一 `Nandina.Reactive` 模块实现与导出
- [x] `State<T>` / `ReadState<T>` / `Computed` / `Effect` / `EffectScope`
- [x] `Prop<T>` 统一组件输入模型
- [x] `StateList<T>` 结构化集合通知
- [x] `tracked_size()` / `tracked_empty()` / `tracked_items()` / `version()` 桥接 API
- [x] `EventSignal` 的 `connect_once`、`ScopedConnection` 与异常后的继续分发语义
- [x] 最小 `batch(...)` 能力与观察者去重 flush
- [x] Widget dirty 剪枝与向上冒泡机制
- [x] `Component::scope_` 生命周期绑定
- [x] `Label` 组件接入 `Prop<T>` / `ReadState<T>`
- [x] 响应式 smoke test 基线

### 收尾项

- [ ] 把 Reactive 1.0 的线程模型、batch 边界与异常优先级写得更明确
- [ ] 视需要补充少量边界测试，但不再扩展新的响应式模型分支

---

## M2：布局系统与真实页面验证

状态：进行中

### 已完成

- [x] `Layout.ixx`、`Row`、`Column`、`Stack`
- [x] `gap()` / `padding()` / `align_items()` / `justify_content()` 基础容器 API
- [x] 页面级接入验证，当前 `DemoPage` 已使用真实列表选择场景跑通布局与响应式组合

### 当前待办

- [ ] 用更多真实页面验证布局边界，而不是只停留在基础容器展示
- [ ] 梳理当前布局 API 与未来 Yoga 接入之间的兼容边界
- [ ] 继续检查测量、嵌套容器和复杂重排场景的行为一致性

---

## M3：样式系统

状态：未开始

### 待办

- [ ] 设计 `ThemeTokens` 结构体与默认主题集合
- [ ] 定义组件读取主题 token 的最小路径
- [ ] 为 Button 等基础组件准备 Variant / 状态样式承载位

---

## M4：完整文字栈

状态：未开始

### 待办

- [ ] 接入 HarfBuzz 文本整形
- [ ] 接入 FreeType 字形光栅化
- [ ] 实现 Glyph Atlas 与缓存策略
- [ ] 让 `Label` 进入真实文本绘制路径

---

## M5：组件库扩展与 Yoga

状态：未开始

### 待办

- [ ] Input / Checkbox / Switch / Dialog / Dropdown 等组件
- [ ] Yoga 集成与内部布局实现替换
- [ ] 保持 Row / Column / Stack 的对外 API 稳定

---

## 近期优先级

1. 完成 M2 的真实页面与布局边界验证。
2. 把 Reactive 1.0 文档与契约说明收紧到当前实现。
3. 在 M3 开始前避免再次大幅修改组件输入模型。
