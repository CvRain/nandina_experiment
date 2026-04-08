# 当前暴露出来的问题整理

这份文档不只是记录 API 不顺手的地方，也记录最近在真实开发中踩到的运行时问题。

目标很明确：

- 开发者在写界面时，应该主要关注结构、样式、状态和交互。
- 不应该频繁手动思考 `std::move` 之后指针是否失效、回调是否还连着、effect 什么时候会立刻执行。
- 如果一个框架要求使用者持续关注所有权和生命周期细节，那说明当前抽象层级还不够高。

## 1. API 风格与样式接口不统一

目前组件的样式和尺寸接口存在明显不统一。

```c++
auto title_box = Nandina::SizedBox::Create();
title_box->set_background(0, 0, 0, 0);
title_box->height(64.0f);

auto title_label = Nandina::Label::Create();
title_label->set_size(Nandina::Size(560.0f, 48.0f));
title_label->font_size(24.0f).text_color(200, 200, 255);
title_label->text("Nandina Counter");
```

问题：

- 有的地方是 `set_size`，有的地方是 `height` / `width`。
- 有的组件支持链式，有的只支持部分链式。
- 使用者必须记住“哪个组件有哪些 setter 可以链式调用”，额外增加心智负担。

期望方向：

- 统一成一套命名规则，例如统一使用 `set_*` 或统一使用 builder 风格。
- 同一类能力在不同组件上尽量同名，例如尺寸、背景、文本、字体、边距。
- `width`、`height`、`size` 可以保留为便捷 API，但底层语义应统一。

## 2. 参数接口不够友好

当前很多样式接口都还是“裸参数”形式，对使用者不够友好。

例如：

- `text_color(200, 200, 255)`
- `font_size(24.0f)`

问题：

- 颜色现在主要靠多个整数传参，不够直观。
- 字体相关信息分散，没有统一的字体样式对象。
- 后续如果支持主题、状态样式、响应式字体缩放，会越来越难扩展。

期望方向：

- 建立更完整的 `Color` 使用方式，支持 RGB、RGBA、Hex 等构造。
- 逐步引入 `Font` 或文本样式对象，统一大小、颜色、字重、行高等属性。
- 尽早确定字体大小语义到底使用 `pixel_size`、`point_size`，还是额外支持相对单位。

## 3. 所有权模型对开发者不友好

这是目前最影响体验的点之一。

当前添加子组件通常要这样写：

```c++
auto title_label = Nandina::Label::Create();
title_box->child(std::move(title_label));
```

问题不是 `std::unique_ptr` 本身，而是：

- 使用者必须明确知道 `std::move` 之后原对象已经失效。
- 一旦在 `add_child` / `child` 之后还继续访问原变量，就很容易踩空。
- 这会把“控件树所有权”直接暴露给业务代码。

最近真实踩到的例子：

- 先 `root->add_child(std::move(number_label));`
- 再在 `effect([label_ptr = number_label.get()] { ... })` 里捕获 `.get()`
- 因为 `number_label` 已经被 move，`.get()` 得到的是空指针，程序启动直接 SIGSEGV

这说明现在的 API 使用方式很容易把开发者引到危险路径上。

期望方向：

- 设计不要求业务层手动管理子控件所有权的 API。
- 例如 `parent->emplace_child<Label>()`、`parent->child(Label::Create())` 返回稳定引用。
- 或者引入更清晰的“挂载后仍可安全配置”的模式，让用户不需要自己推导生命周期。

## 4. 生命周期与 effect 注册时机过于脆弱

当前 `effect()` 在注册时会立即执行。

这本身不是问题，但与当前所有权模型组合起来会很危险：

- 如果 effect 捕获了一个已经失效的控件指针，会在初始化阶段立刻崩溃。
- 如果 effect 依赖的控件还没挂载完成，也容易出现不符合预期的行为。

对于框架使用者来说，这意味着：

- 不仅要知道“控件何时 move”，还要知道“effect 何时首跑”。
- 这会让开发者在写界面时更像是在排查生命周期，而不是在描述 UI。

期望方向：

- 提供更安全的绑定方式，例如 `bind_text(label, state)` 这类高阶 API。
- 让“组件创建 + 挂载 + 绑定”成为一个更难写错的流程。
- 尽量避免业务层直接处理裸指针捕获。

## 5. 事件连接语义不直观

最近还暴露过一个比较典型的问题：

```c++
button->on_click([this] {
        count_.set(count_.get() + 1);
});
```

表面看非常自然，但之前实际不会触发。

原因是：

- `on_click()` 返回 `Connection`
- 如果返回值不保存，临时对象会立即析构
- 析构函数之前还会自动 `disconnect()`
- 结果就是回调刚注册完就断开了

这个问题已经修过，但它说明一件事：

- 当前信号系统的语义和开发者直觉不一致。
- Qt 风格写法看起来成立，但内部行为完全不同。

期望方向：

- 默认写法应该天然可用，不要求保存连接对象。
- 如果确实需要显式解绑，应提供更清晰的高级控制接口，而不是把默认路径设计成陷阱。

## 6. 响应式系统还缺少足够稳固的约束

最近还踩到过一个响应式核心问题：

- 同一个 `Effect` 在一次执行中多次读取同一个 `State`
- 旧实现会重复注册依赖
- 下一次 `set()` 后 effect 触发次数按 2、4、8、16 指数增长

这个问题已经修过，但它说明：

- 当前响应式系统仍然缺少足够多的稳定性校验
- 核心层还需要更多测试覆盖，而不是只靠页面试错

建议：

- 为 `State` / `Effect` / `Computed` 单独补测试
- 覆盖重复读取、嵌套 effect、生命周期销毁、重复订阅去重等场景

## 7. 布局系统还不够“可依赖”

目前布局是另一个主要问题来源。

现状：

- `Row` / `Column` 是两段式分配
- `Spacer` / `Expanded` 能工作
- 但嵌套容器并不会自动稳定递归布局
- 有些页面为了正确显示，只能退回绝对定位

这会带来一个直接后果：

- 开发者本来应该只描述“我想让它居中、我想让按钮在标签下方”
- 最后却要自己算坐标、手动调用 `layout()`、绕开某些容器组合

期望方向：

- 先把布局系统做成可预测、可递归、可嵌套。
- 让“标题栏 + 中间内容 + 按钮行”这种典型结构可以只靠容器表达。
- 绝对布局应该是能力之一，而不是在布局系统不稳定时的逃生通道。

## 8. Window / Layer 能力对普通页面来说仍然偏底层

这一点之前也已经体现出来：

- Layer 架构本身是有价值的
- 但对普通业务页面来说，直接接触 layer 仍然偏底层

现在虽然已经有 `set_background`、`set_content`、`add_child` 之类的高层接口，
但整体方向仍然应该是：

- 普通窗口开发默认只关心内容区、背景、浮层
- 更底层的 layer 细节尽量隐藏在 WindowController 内部

## 9. 当前最需要优先优化的顺序

结合最近踩到的问题，建议优化顺序如下：

1. 先解决所有权与生命周期易错问题。
原因：这类问题会直接导致 SIGSEGV，属于开发体验里最糟糕的一类。

2. 再解决布局系统的递归与嵌套可预测性。
原因：这决定了开发者能不能专注写界面，而不是退回绝对坐标。

3. 再统一组件样式 API 和链式调用风格。
原因：这是日常使用频率最高的表层体验问题。

4. 最后再逐步把颜色、字体、主题这些参数系统抽象完整。
原因：这类体验很重要，但不如前两者影响“能不能稳定开发”。

## 10. 一个更核心的设计目标

如果后续要继续优化，我觉得需要始终围绕一个目标判断：

- 开发者写页面时，是否主要在描述 UI 本身
- 还是仍然在手动管理控件树、所有权、生命周期和触发时机

如果答案偏向后者，那说明当前抽象还不够高。

Nandina 后续应该尽量朝这个方向收敛：

- 用更高层的 API 把所有权和生命周期细节藏起来
- 让开发者主要写“这个组件长什么样、放在哪里、和什么状态绑定”
- 而不是不断思考 `move` 之后还能不能用、effect 什么时候首跑、signal 会不会自动断开

## 11. 理想状态下的 CounterPage 应该怎么写

如果后续这些问题都逐步解决，那么我希望最终开发者写一个简单页面时，代码体验应该接近下面这样：

```c++
class CounterPage : public Nandina::Page {
public:
        static auto Create() -> std::unique_ptr<CounterPage>;

        auto build() -> Nandina::WidgetPtr override;

private:
        Nandina::State<int> count_{0};
};

auto CounterPage::build() -> Nandina::WidgetPtr {
        auto page = Nandina::Column::Create()
                ->set_size({640.0f, 480.0f})
                .set_background(Nandina::Color::Hex(0x323544))
                .set_padding_top(15.0f)
                .set_padding_bottom(15.0f)
                .set_padding_left(20.0f)
                .set_padding_right(20.0f)
                .set_gap(24.0f);

        page->add(
                Nandina::Label::Create()
                        ->set_size({560.0f, 48.0f})
                        .set_text("Nandina Counter")
                        .set_font(Nandina::Font::Pixels(24.0f).color("#c8c8ff"))
        );
    
        /*
         page->add(
                Nandina::Label::Create()
                        ->set_size({560.0f, 48.0f})
                        .set_text("Nandina Counter")
                        .set_font(Nandina::Font{
                            .pixel_size = 24.0f,
                            .color = Nandina::Color{0xc8c8ff}
                            .weight = Nandina::FontWeight::Regular
                            // ... 其他字体属性
                        })
        );
         */

        auto center = page->add(
                Nandina::Column::Create()
                        ->expand()
                        .align_items(Nandina::Align::center)
                        .justify_content(Nandina::Align::center)
                        .gap(20.0f)
        );

        auto number_label = center->add(
                Nandina::Label::Create()
                        ->set_size({240.0f, 64.0f})
                        .set_font(Nandina::Font::Pixels(48.0f).color("#dcf0e8"))
                        .bind_text(count_, [](int value) {
                                return std::format("{}", value);
                        })
        );

        center->add(
                Nandina::Row::Create()
                        ->align_items(Nandina::Align::center)
                        .justify_content(Nandina::Align::center)
                        .gap(16.0f)
                        .add(
                                Nandina::Button::Create()
                                        ->set_size({120.0f, 60.0f})
                                        .set_text("-1")
                                        .set_background("#ea999c")
                                        .on_click([this] {
                                                count_.set(count_.get() - 1);
                                        })
                        )
                        .add(
                                Nandina::Button::Create()
                                        ->set_size({120.0f, 60.0f})
                                        .set_text("+1")
                                        .set_background("#ea999c")
                                        .on_click([this] {
                                                count_.set(count_.get() + 1);
                                        })
                        )
        );

        return page;
}
```

这段代码想表达的不是“语法必须完全长这样”，而是理想体验应该具备下面这些特征：

- 页面主要在描述结构和样式，而不是处理所有权。
- `add()` 之后仍然能拿到稳定引用或句柄，不需要自己保存裸指针。
- 状态绑定使用 `bind_text()` 这类高阶 API，而不是手写 `effect + 裸指针捕获`。
- 按钮点击直接写 lambda 即可，不需要额外保存连接对象。
- 颜色和字体使用更高层的值对象，而不是大量裸参数。
- 居中、间距、扩展这些布局语义可以直接表达，不需要退回绝对坐标。

如果后续要逐步优化，我觉得这段 CounterPage 可以作为一个很好的目标样例：

- 每推进一部分框架抽象，就看是否能让真实页面代码更接近这段写法。
- 如果某项改动不能让页面代码更短、更稳、更像在描述 UI，那它大概率还不是最优方向。