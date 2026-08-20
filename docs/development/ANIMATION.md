# 动画 / 过渡系统（Animation & Transitions）

> 状态：1.0 已落地「最小 Tween + easing 曲线 + Tabs/Dialog 两点示范」；
> 1.1 九个交付单元全部完成：公共 typed visual property path、
> `AnimatedProperty<T> + Behavior<T>` 值语义、场景树 `AnimationHost`，
> Builder `.bind/.behavior` 与 typed property endpoint、`NanNode2D::local_opacity`
> （含移除 `StyleContext.opacity` 双重相乘瑕疵）、全局 reduced-motion 下沉、
> Label/Button 纵向示范、Tabs/Dialog 迁移 + Dialog 状态机、parallel/sequential/stagger
> 组合器、spring + keyframes，以及 router transition（页面退出保留生命周期）。
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
- `animation/property_endpoint.hpp`：primitive 可复用的 owner-aware endpoint；未挂载时初始化
  直跳，挂载后把 target 交给所属场景树 Host，并负责行为变更和 override 清除时注销轨道。
- 两个示范：
  - `Tabs` 下划线指示条 `indicator_x_/indicator_width_` 切换时滑动（`medium_duration`）。
  - `Dialog` 打开时 scrim + 面板淡入（`long_duration` + `ease_out`）。

**仍待后续单元解决**：
- 组件语义转场的 motion slot 定制 DSL（`builder.motion(...)`）尚未暴露；Tabs/Dialog 内部已用
  `AnimatedProperty` + Host，但开发者定制「指示器移动」「Dialog 出入场」的入口待后续单元。
- 组合器与 router transition（单元 8）：parallel/sequential/stagger 与页面退出保留生命周期。
- spring / keyframes（单元 9）。

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

第四单元已实现该入口：`PropertyEndpoint<T>` 持有稳定 `AnimatedProperty<T>`，Box/Text
primitive 的字段代理只做校验与转发。`BuildContext::make<T>()` 返回的 Builder 携带当前
`ReactiveScope`，因此 `.bind(path, source)` 的 effect 仍由页面/组件/region scope 管理；
低层 `authoring::make<T>()` 没有隐式 scope，不能单独建立 binding。`.behavior()` 不拥有
订阅或时钟，只向同一 endpoint 安装类型匹配的 `Behavior<T>`。

视觉实例属性属于本地显式值，级联优先级高于继承 `StyleContext`、组件 recipe 与 theme token；
例如 `Button::set_font_size(float)` 与 `visual::label.font_size` 写入同一 presentation 存储，
不会形成两套互相覆盖的固定字号状态。百分比字号是依赖最终布局高度的另一种输入形态。

### 3.3 AnimationHost 与帧阶段

`AnimatedProperty` 不自行要求组件覆写 `on_process()`。活跃属性注册到场景树拥有的
`AnimationHost`，完成、取消或 owner 退出树时自动注销。Host 只遍历活跃动画，不扫描所有节点。

第三单元采用以下运行时契约：

- 每个 `NanSceneTree` 独占一个 Host；不同窗口/场景树的时钟和轨道不共享。
- `set_target(owner, property, target, dirty_flags)` 是 Host 接入点；以 property 地址作为
  轨道 identity，连续 retarget 更新原轨道而不是重复注册。
- Host 仅持有弱 owner；节点 `_propagate_exit_tree()` 时同步完成并取消其全部轨道，保证属性
  不残留 `is_animating()` 状态，keep-alive 但已卸载的页面不会继续推进或重挂载后续跑。
- `advance(dt)` 使用调用方提供的帧增量，测试可直接注入确定性 fake/manual clock；不读取全局时间。
- 每次 presentation value 确实变化后才传播该属性声明的 `DirtyFlags`；完成轨道当帧移除。
- 当前模板 API 要求 `AnimatedProperty` 与 owner 生命周期一致（通常是组件/presentation 成员）；
  后续 Builder 只组合该协议，不再建立第二套轨道所有权。
- 全局 reduced-motion（单元 6）：Host 通过 `tree_->theme_manager()->reduced_motion()` 读取策略。
  `set_target` 归约时 `property.finish()` 直跳不注册轨道；`advance()` 归约时 `clear()` 完成全部
  在途轨道。无 ThemeManager 时视为未归约。策略切换无需 revision 通知，下一帧 `advance` 自会收敛。

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

单元 9 已落地 spring：`SpringSpec`（stiffness/damping/mass，拒绝非法参数）与 `Spring<T>`
（半隐式 Euler 积分、settle 判定、retarget 保留速度、`finish` 直跳）。`AnimatedProperty<T>`
新增 `set_spring/clear_spring`，与 `Behavior` 互斥，浮点类型才启用（`NanColor` 等非浮点用
`SpringMemberSelector` 惰性选择占位类型，避免约束不满足）。spring 复用同一 Host/时钟/取消/
retarget，不引入第二套调度器。

单元 9 同时落地 keyframes：`Keyframe<T>`（time + value，时间严格递增且首帧为 0）与
`Keyframes<T>`（按时间线性插值，复用 `lerp` 故算术类型与 `NanColor` 都可用；插值结果缓存
使 `value()` 返回稳定引用）。`AnimatedProperty<T>` 新增 `set_keyframes/clear_keyframes`，与
`Behavior`/`Spring` 互斥，`set_target` 会清除 keyframes 回落 tween/spring。builder
`.behavior(motion::spring(...))` / `.keyframes(...)` DSL 入口仍留待后续。

### 3.5 组件内部转场

并非所有动效都应拆成公开数值属性。Tabs 指示器、Dialog 出入场属于组件语义转场，公开
`indicator_x` / `indicator_width` 会泄漏实现。此类能力使用类型化 motion slot：

```cpp
builder.motion(Tabs::indicator_motion, motion::spring(...));
builder.motion(Dialog::visibility_motion, motion::tween(...));
```

组件内部可用一个 `AnimatedProperty<NanRect>` 或 parallel group 实现 slot，但开发者定制的是
“指示器移动”“Dialog 出入场”，而不是私有几何字段。

单元 7 已落地内部迁移：Tabs 的 `indicator_x_/indicator_width_` 与 Dialog 的 `fade_` 均改为
`AnimatedProperty<float>` 并由场景树 Host 推进，删除手写 `on_process` tick；Dialog 增加
`opening/opened/closing/closed` 状态机，`close()` 延迟到淡出完成才隐藏并触发 `on_close_`，
内容子节点通过覆写 `Dialog::local_opacity()`（`NanNode2D::local_opacity() × fade_.value()`）
随面板整体淡入淡出。公开的 `builder.motion(...)` slot 定制仍留待后续单元。

### 3.6 组合（1.1 后段）

```cpp
parallel(a, b) / sequential(a, b) / stagger(items, interval)
```

用于「面板淡入 + 上移」等复合转场；router 页面切换复用同一 Host、时钟和取消语义。

单元 8 已落地组合器 `animation::Group`：一个 Group 聚合多个类型擦除的 clip（每个 clip 包装
一个 `AnimatedProperty<T>` 的目标 + `Behavior<T>`），`Group::parallel/sequential/stagger` 只决定
每个 clip 的触发谓词（全立即 / 前一个完成 / 固定间隔）。`AnimationHost::run(owner, group)`
把 Group 作为一条轨道推进，复用同一时钟、归约动效与 owner 取消语义；Group 按值移交、由
Host 持有，规避了「局部 Group 先于场景树析构」的悬垂。Group 只可移动（`sequential` 的
ready 谓词用指针引用相邻 clip，move 保留缓冲地址、copy 会悬垂）。

单元 8 同时落地 router transition（`NanRouter::set_transition_enabled`，默认关闭=即时切换）：
开启后每页根包一层 `PageFrame`（`AnimatedProperty<float> opacity` + 覆写 `local_opacity()`），
push 淡入、pop/replace 淡出；被替换页面移入 `exiting_` 列表，其 scope/async 在淡出期间保留，
`PageHost::on_process` 轮询淡出完成后才清生命周期并把 detach 延迟到 tree_commit（`remove_child`
不能出现在 process 阶段）。关闭转场时行为与原先完全一致（host 直接持有页面根）。

## 4. 开发流程与交付顺序

每一步必须是可独立评审、可测试的纵向单元；在 Host 和属性协议稳定前，不继续给组件堆叠
手写 Tween。

| 顺序 | 交付单元 | 验收条件 |
|---|---|---|
| 1（完成） | 公共 visual property path + primitive part 暴露 | 支持/不支持的属性组合由 concept 编译期判定；第三方组件无需中心注册 |
| 2（完成） | `AnimatedProperty<T>` + `Behavior<T>` | target/value 分离、直跳、retarget、负 dt/越界输入测试 |
| 3（完成） | `AnimationHost` + animation frame phase | 活跃注册、owner 卸载取消、fake clock、正确 DirtyFlags |
| 4（完成） | Builder `.bind(path, source)` / `.behavior(path, spec)` | 命令式与 DSL 写入同一 target；错误节点/属性组合无法编译 |
| 5（完成） | `NanNode2D::local_opacity` | 子树只乘一次节点 alpha；不产生随深度指数衰减 |
| 6（完成） | Label 颜色 + Button container radius 纵向示范 | OKLCH 颜色插值、圆角过渡、reduced motion、绘制结果测试 |
| 7（完成） | Tabs/Dialog 迁移 | 删除组件手写 tick；Dialog opening/opened/closing/closed 状态机 |
| 8（完成） | 组合器与 router transition | parallel/sequential/stagger 落地；页面退出保留生命周期的 router transition 落地 |
| 9（完成） | spring / keyframes | spring + keyframes 落地（统一时钟/取消/retarget，不引入第二套调度器） |

每个单元执行：文档状态同步 → 最小实现 → focused tests → 全量测试 → 示例视觉复核 → 提交。

## 5. Per-node opacity

现有 `DrawContext` 已有 inherited opacity 通道，但旧实现把 `StyleContext.opacity` 作为
「默认继承」属性解析（`resolve_style_value(..., inherits_by_default=true)`），再在绘制遍历里
乘一次，导致深层树中 opacity 随继承层级重复指数衰减（父 0.5、子无覆盖 → 子实际 0.25）。
1.1 修复并增加独立的 `NanNode2D::local_opacity`：

```text
effective opacity = parent effective opacity × node local opacity
```

修复决策（单元 5）：

- 删除 `StyleContext.opacity` / `ResolvedStyleContext.opacity` / `opacity_from_context`
  这套与节点 alpha 重复的继承属性（其运行时唯一消费者就是绘制乘法），不留冗余旧接口。
- `NanNode` 新增虚钩子 `local_opacity()`（基类恒 `1.0`），`NanNode2D` 覆写返回局部 alpha，
  使绘制遍历对任意节点都能「只乘一次局部值」而不依赖 RTTI。
- `NanNode2D::local_opacity_` 默认 `1.0`；`set_local_opacity()` 拒绝非有限值、clamp 到
  `[0,1]`，值未变时 no-op。
- 修改只产生 paint invalidation：`NanNode::mark_paint_dirty()` 沿祖先链找到最近的
  `NanControl`（或自身）置 `DirtyFlags::paint`，不触碰 layout/semantics。
- 不影响可见性、输入、semantics：这些仍由显式转场状态决定。

每个节点只乘一次，影响整个子树绘制；这是 Dialog 内容淡入、页面转场和任意子树动画的基础。

验证（headless）：`render_tests` 覆盖父子透明度（父 0.5、子无覆盖 → 0.5 而非 0.25）、
三层 0.5 嵌套 → 0.125、兄弟隔离与上下文恢复、绘制 alpha 结果；`scene_tests` 覆盖默认值
1.0、clamp 到 [0,1]、非有限值抛错、透明度不改 visibility，以及 `set_local_opacity` 仅置
paint dirty（不触 layout/semantics）。全量 `meson test` 40/40 通过。

## 6. 与 reactive 的关系

reactive graph 管逻辑 target，animation Host 管 presentation value。动画逐帧更新不得通知
`Signal` 或重新运行普通 effect，否则会把纯绘制插值放大为每帧响应式/布局风暴。

```text
Signal / ordinary setter → property target
property target + Behavior → AnimationHost track
track current value → dirty propagation → layout/paint
```

binding 的 scope 继续拥有订阅生命周期；AnimationHost 通过弱 owner 或节点退出通知取消 track。
