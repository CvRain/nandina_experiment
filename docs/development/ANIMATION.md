# 动画 / 过渡系统（Animation & Transitions）

> 状态：1.0 已落地「最小 Tween + easing 曲线 + Tabs/Dialog 两点示范」；
> 1.1 前两个交付单元已完成：公共 typed visual property path，以及
> `AnimatedProperty<T> + Behavior<T>` 值语义。下一单元是 `AnimationHost`。
> 关联：Phase 8 Deferred Work「general tween/animation system」。

## 1. 目标

1. 让组件属性变化可以**平滑过渡**（Tabs 下划线滑动、Dialog 淡入淡出、颜色/位置/尺寸渐变）。
2. 动画是**声明式、可定制**的——类似 QML 的 `Behavior on x { NumberAnimation { ... } }`，
   而不是散落在每个 widget 里的手写 Tween 时序。
3. 尊重全局 `reduced_motion()`（归约动效下直跳）。

## 2. 现状

- `animation/easing.hpp`：`linear / ease_in / ease_out / ease_in_out` 四条缓动曲线。
- `animation/tween.hpp`：泛型 `Tween<T>`（`start/tick/finish/reset/value/is_finished`），
  算术类型自动 `lerp`，`NanColor` 使用 OKLCH 最短色相弧插值。
- `animation/behavior.hpp`：类型化属性的时长、缓动与启用策略；拒绝负数及非有限时长。
- `animation/animated_property.hpp`：逻辑 `target` 与呈现 `value` 分离；无 Behavior、禁用、
  零时长时直跳，中途换目标从当前 `value` 连续 retarget。
- 两个示范：
  - `Tabs` 下划线指示条 `indicator_x_/indicator_width_` 切换时滑动（`medium_duration`）。
  - `Dialog` 打开时 scrim + 面板淡入（`long_duration` + `ease_out`）。

**仍待后续单元解决**：
- `AnimatedProperty` 目前是独立值对象，尚未接入场景树 `AnimationHost`；现有 Tabs/Dialog
  仍手写 `on_process()`。
- `Dialog` 内容子节点不淡入（无 per-node opacity，见 §5）。
- `Dialog` 淡出未做（需把 close 延迟到淡出完成，涉及模态状态机）。

## 3. 目标架构（QML 式，1.1）

动画不能建立第二套 widget 对象模型，也不能要求每个组件重复声明
`Button::corner_radius_property`、`Label::color_property`。Nandina 的组件由共享 primitive、
token 和 recipe 组装，因此属性协议也按同一结构分层：

```text
标准部件（part / slot） + 标准字段（field） = 类型化视觉属性路径

visual::label.color
visual::label.font_size
visual::container.fill
visual::container.border_color
visual::container.border_width
visual::container.radius
visual::focus_ring.color
visual::shadow.spread
scene::opacity
scene::position
```

`color` 只是值类型，不足以标识属性；Button 同时可能拥有 container、label、icon、focus、
ripple 等多个颜色。路径必须保留部件语义，同时与具体组件类型解耦。

### 3.1 公共视觉属性协议

组件只暴露自己由哪些标准部件组成，字段读写由 primitive 提供：

```cpp
class Button : public primitives::Pressable {
public:
    auto visual_part(visual::container_t) -> primitives::BoxPresentation&;
    auto visual_part(visual::label_t) -> primitives::TextPresentation&;
};
```

`BoxPresentation` 统一提供 fill / border / radius 等字段；`TextPresentation` 统一提供组件内部
文本 part 的 color / font-size；独立 `Text` primitive 自身也实现同一字段协议。
第三方组件只需要组合并暴露这些 part，不向 BuildContext 或全局组件目录登记属性。

属性路径和节点类型在 DSL 中均为静态类型，因此错误组合必须由 concept 在编译期拒绝：

```cpp
static_assert(property::Writable<Button, decltype(visual::container.radius)>);
static_assert(!property::Writable<Label, decltype(visual::container.radius)>);
```

不使用字符串查找、RTTI 或“尝试后抛异常”的运行时协议。

### 3.2 带动画的属性

属性需要区分逻辑目标与当前呈现值：

```cpp
namespace nandina::animation {
    template<typename T>
    class AnimatedProperty {
    public:
        void set_target(T value);
        [[nodiscard]] auto target() const -> const T&;
        [[nodiscard]] auto value() const -> const T&;
        void set_behavior(Behavior<T> behavior);
    };
}
```

- `target()` 是业务/语义状态，setter 或 reactive binding 写入后立即改变。
- `value()` 是绘制、变换或布局读取的当前呈现值，由动画逐帧逼近 target。
- 同目标写入 no-op；中途改目标默认从当前 value 连续 retarget。
- reduced motion、零时长或未安装 Behavior 时直接令 value = target。

普通 setter、DSL binding 和 imperative API 必须汇入同一个属性入口：

```cpp
label->set_color(color);                            // 命令式
ui.bind(label, visual::label.color, color_signal); // 响应式目标
builder.behavior(visual::label.color, tween);      // 变化行为
```

视觉实例属性属于本地显式值，级联优先级高于继承 `StyleContext`、组件 recipe 与 theme token；
例如 `Button::set_font_size(float)` 与 `visual::label.font_size` 写入同一 presentation 存储，
不会形成两套互相覆盖的固定字号状态。百分比字号是依赖最终布局高度的另一种输入形态。

### 3.3 AnimationHost 与帧阶段

`AnimatedProperty` 不自行要求组件覆写 `on_process()`。活跃属性注册到场景树拥有的
`AnimationHost`，完成、取消或 owner 退出树时自动注销。Host 只遍历活跃动画，不扫描所有节点。

动画阶段位于 reactive 之后、layout 之前：

```text
input → tasks → process → tree_commit → physics → reactive
      → animation → layout → semantics → paint → dispose
```

这样本帧 reactive 产生的新 target 可立即建立动画；layout/paint 同帧看到一致的 presentation。
每条属性路径声明自己的失效域（paint / transform / layout / semantics），Host 更新 value 后
统一触发正确脏标记。

### 3.4 DSL 目标形态

```cpp
ui.make<widget::Label>("颜色会平滑变化")
    .bind(visual::label.color, label_color)
    .behavior(
        visual::label.color,
        motion::tween(0.24F)
            .easing(ease::standard)
            .color_space(motion::ColorSpace::oklch)
    );

ui.make<widget::Button>("切换状态")
    .bind(visual::container.radius, button_radius)
    .behavior(
        visual::container.radius,
        motion::spring().stiffness(280.0F).damping(26.0F)
    );
```

Behavior 覆盖优先级固定为：实例覆盖 → 组件 recipe motion → theme motion token → 立即更新；
全局 reduced-motion policy 在最外层强制直跳。resolved theme 值只作为目标来源，不回写 recipe。

### 3.5 组件内部转场

并非所有动效都应拆成公开数值属性。Tabs 指示器、Dialog 出入场属于组件语义转场，公开
`indicator_x` / `indicator_width` 会泄漏实现。此类能力使用类型化 motion slot：

```cpp
builder.motion(Tabs::indicator_motion, motion::spring(...));
builder.motion(Dialog::visibility_motion, motion::tween(...));
```

组件内部可用一个 `AnimatedProperty<NanRect>` 或 parallel group 实现 slot，但开发者定制的是
“指示器移动”“Dialog 出入场”，而不是私有几何字段。

### 3.6 组合（1.1 后段）

```cpp
parallel(a, b) / sequential(a, b) / stagger(items, interval)
```

用于「面板淡入 + 上移」等复合转场；router 页面切换复用同一 Host、时钟和取消语义。

## 4. 开发流程与交付顺序

每一步必须是可独立评审、可测试的纵向单元；在 Host 和属性协议稳定前，不继续给组件堆叠
手写 Tween。

| 顺序 | 交付单元 | 验收条件 |
|---|---|---|
| 1（完成） | 公共 visual property path + primitive part 暴露 | 支持/不支持的属性组合由 concept 编译期判定；第三方组件无需中心注册 |
| 2（完成） | `AnimatedProperty<T>` + `Behavior<T>` | target/value 分离、直跳、retarget、负 dt/越界输入测试 |
| 3 | `AnimationHost` + animation frame phase | 活跃注册、owner 卸载取消、fake clock、正确 DirtyFlags |
| 4 | Builder `.bind(path, source)` / `.behavior(path, spec)` | 命令式与 DSL 写入同一 target；错误节点/属性组合无法编译 |
| 5 | `NanNode2D::local_opacity` | 子树只乘一次节点 alpha；不产生随深度指数衰减 |
| 6 | Label 颜色 + Button container radius 纵向示范 | OKLCH 颜色插值、圆角过渡、reduced motion、绘制结果测试 |
| 7 | Tabs/Dialog 迁移 | 删除组件手写 tick；Dialog opening/opened/closing/closed 状态机 |
| 8 | 组合器与 router transition | parallel/sequential/stagger 与页面退出保留生命周期 |
| 9 | spring / keyframes | 统一时钟、取消、retarget，不引入第二套调度器 |

每个单元执行：文档状态同步 → 最小实现 → focused tests → 全量测试 → 示例视觉复核 → 提交。

## 5. Per-node opacity

现有 `DrawContext` 已有 inherited opacity 通道，但 `StyleContext.opacity` 会按继承层级重复解析并
相乘，不等同于节点局部 alpha。1.1 增加独立 `NanNode2D::local_opacity`：

```text
effective opacity = parent effective opacity × node local opacity
```

它只在当前节点乘一次，影响整个子树绘制；可见性、输入和 semantics 仍由显式转场状态决定。
这是 Dialog 内容淡入、页面转场和任意子树动画的基础。

## 6. 与 reactive 的关系

reactive graph 管逻辑 target，animation Host 管 presentation value。动画逐帧更新不得通知
`Signal` 或重新运行普通 effect，否则会把纯绘制插值放大为每帧响应式/布局风暴。

```text
Signal / ordinary setter → property target
property target + Behavior → AnimationHost track
track current value → dirty propagation → layout/paint
```

binding 的 scope 继续拥有订阅生命周期；AnimationHost 通过弱 owner 或节点退出通知取消 track。
