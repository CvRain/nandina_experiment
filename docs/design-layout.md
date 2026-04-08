# Layout 设计草案

## 1. 目标

这份文档描述的是 Nandina 在理想状态下的布局系统设计目标，不完全等同于当前实现。

布局系统需要解决的核心问题：

- 开发者只描述“结构关系”和“排布意图”，而不是自己计算坐标。
- Widget 在内容变化、窗口缩放、显示隐藏切换后可以自动重新排布。
- 常见页面结构可以通过组合布局表达，而不需要退回绝对定位。
- 布局系统要和响应式系统协同工作，内容变化后自动触发布局失效与重排。

理想情况下，给某个 Widget 设置布局以后，布局应自动负责：

- 定位子组件
- 推导合理的默认尺寸
- 推导合理的最小尺寸
- 处理窗口和父容器尺寸变化
- 在内容变化时自动触发布局更新
- 在子组件隐藏、显示、删除时自动更新

## 2. 设计原则

### 2.1 一个 Widget 只管理一个布局

每个容器 Widget 同一时间只能设置一个布局：

```c++
auto page = Nandina::Widget::Create();
auto layout = Nandina::VerticalLayout::Create();
page->set_layout(std::move(layout));
```

布局负责管理子组件的几何信息，但子组件的生命周期仍然由 Widget 树统一管理。

### 2.2 布局负责位置，Widget 负责样式和行为

布局组件不关心按钮颜色、文本内容、事件逻辑。
布局只关心：

- 子项顺序
- 对齐方式
- 间距
- 外边距/内边距
- 拉伸因子
- 行列索引与跨行跨列

### 2.3 默认自动重排

只要以下任一条件发生，布局系统都应该自动标记失效并在下一帧重排：

- 父容器尺寸变化
- 子组件尺寸提示变化
- 子组件文本变化导致推荐尺寸变化
- 子组件显示/隐藏变化
- 子组件增删变化
- 布局参数变化，例如 gap、padding、alignment

### 2.4 绝对布局是能力，不是退路

Nandina 仍然应该保留自由定位能力，例如 `FreeWidget` 或显式 absolute positioning。
但它应该作为补充能力存在，而不是因为布局系统不够稳定时的主要写法。

## 3. 推荐布局组件清单

### 3.1 第一阶段必须具备的布局组件

| 布局组件 | 作用 | 典型场景 |
| :-- | :-- | :-- |
| `BoxLayout` | 一维线性布局基类 | 统一横向/纵向布局行为 |
| `HorizontalLayout` | 水平排列子组件 | 工具栏、按钮行、表单行 |
| `VerticalLayout` | 垂直排列子组件 | 页面主结构、卡片列表 |
| `GridLayout` | 按行列排布 | 面板、宫格、设置项矩阵 |
| `StackLayout` | 子组件叠放 | overlay、状态切换、层叠内容 |
| `Spacer` | 弹性空白占位 | 居中、推挤、分组间距 |
| `Expanded` | 占据剩余空间 | 主内容区域、自适应伸缩 |
| `SizedBox` | 固定尺寸约束 | 固定高标题栏、固定宽按钮 |
| `Padding` | 提供内边距 | 页面留白、组件包裹 |
| `Center` | 单子项居中容器 | 中央标题、空态页、数字显示 |

### 3.2 第二阶段建议补充的布局组件

| 布局组件 | 作用 | 典型场景 |
| :-- | :-- | :-- |
| `FlowLayout` | 自动换行布局 | 标签云、图片流、按钮池 |
| `FormLayout` | 标签-字段双列布局 | 设置页、输入表单 |
| `AnchorLayout` | 相对父容器边界锚定 | HUD、角标、浮动工具区 |
| `SplitLayout` | 可拖拽分栏 | IDE、左右面板、文件浏览器 |

## 4. 统一接口约定

为了避免当前 API 风格不统一，理想状态下布局组件应共享一组尽量一致的接口。

### 4.1 LayoutContainer 统一接口

所有布局容器都应至少具备以下接口：

| 接口 | 作用 |
| :-- | :-- |
| `set_gap(float)` | 设置子项之间的间距 |
| `set_padding(float)` | 设置四周统一内边距 |
| `set_padding(horizontal, vertical)` | 设置水平/垂直内边距 |
| `set_padding(left, top, right, bottom)` | 精确设置四边内边距 |
| `set_align_items(Align)` | 设置交叉轴对齐 |
| `set_justify_content(JustifyContent)` | 设置主轴对齐 |
| `add_child(widget)` | 将子组件加入布局末尾 |
| `insert_child(index, widget)` | 在指定位置插入子组件 |
| `remove_child(widget)` | 从布局中移除子组件 |
| `clear()` | 清空所有子组件 |
| `invalidate_layout()` | 手动标记布局失效 |
| `layout()` | 立即执行一次布局计算 |

推荐统一命名为 `set_*` 风格，避免有的组件使用 `gap()`，有的使用 `set_gap()`。

### 4.2 对齐相关枚举

建议统一引入两个枚举，而不是混用单一枚举：

```c++
enum class Align {
    Start,
    Center,
    End,
    Stretch
};

enum class JustifyContent {
    Start,
    Center,
    End,
    SpaceBetween,
    SpaceAround,
    SpaceEvenly
};
```

这样可读性更强，也更接近主流布局系统的语义。

### 4.3 子项布局参数

除了直接 `add_child(widget)`，布局系统还应支持带参数的子项添加方式。

例如：

```c++
layout->add_child(
    Nandina::Button::Create(),
    Nandina::LayoutParams{}
        .set_flex(1)
        .set_align_self(Nandina::Align::Center)
);
```

理想状态下，`LayoutParams` 至少应支持：

- `flex`
- `min_width` / `min_height`
- `max_width` / `max_height`
- `preferred_width` / `preferred_height`
- `align_self`
- `margin`
- Grid 专用的 `row` / `column` / `row_span` / `column_span`

## 5. 每种布局组件的职责与接口

### 5.1 BoxLayout

一维布局基类，不直接给业务层使用，负责抽象横向/纵向的公共逻辑。

公共能力：

- gap
- padding
- flex 分配
- main axis / cross axis 对齐
- 子项最小/首选尺寸计算

它应作为 `HorizontalLayout` 和 `VerticalLayout` 的公共基类。

### 5.2 HorizontalLayout

用于水平方向排列子组件。

典型接口：

```c++
auto layout = Nandina::HorizontalLayout::Create();
layout->set_gap(12.0f)
      .set_padding(16.0f)
      .set_align_items(Nandina::Align::Center)
      .set_justify_content(Nandina::JustifyContent::Center);
```

适用场景：

- 按钮组
- 工具栏
- 标签 + 按钮行
- 左右分栏中的单行区域

示例：

```c++
auto actions = Nandina::HorizontalLayout::Create();
actions->set_gap(16.0f)
       .set_align_items(Nandina::Align::Center)
       .set_justify_content(Nandina::JustifyContent::Center);

actions->add_child(
    Nandina::Button::Create()->set_text("Cancel")
);
actions->add_child(
    Nandina::Button::Create()->set_text("Confirm")
);
```

### 5.3 VerticalLayout

用于垂直方向排列子组件。

典型接口：

```c++
auto layout = Nandina::VerticalLayout::Create();
layout->set_gap(24.0f)
      .set_padding(40.0f)
      .set_align_items(Nandina::Align::Stretch)
      .set_justify_content(Nandina::JustifyContent::Start);
```

适用场景：

- 页面主结构
- 设置页分段
- 卡片列表
- 标题 + 内容 + 操作区

示例：

```c++
auto page_layout = Nandina::VerticalLayout::Create();
page_layout->set_gap(20.0f)
           .set_padding(24.0f);

page_layout->add_child(
    Nandina::Label::Create()->set_text("Settings")
);
page_layout->add_child(
    Nandina::Widget::Create()->set_layout(Nandina::FormLayout::Create())
);
page_layout->add_child(
    Nandina::HorizontalLayout::Create()
        ->set_justify_content(Nandina::JustifyContent::End)
);
```

### 5.4 GridLayout

按行列组织子组件。

典型接口：

| 接口 | 作用 |
| :-- | :-- |
| `set_column_count(int)` | 设置列数 |
| `set_row_count(int)` | 设置行数，可选 |
| `set_gap(float)` | 设置统一间距 |
| `set_row_gap(float)` | 设置行间距 |
| `set_column_gap(float)` | 设置列间距 |
| `set_column_stretch(column, flex)` | 设置列拉伸权重 |
| `set_row_stretch(row, flex)` | 设置行拉伸权重 |
| `add_child(widget, row, column)` | 在指定单元格添加子组件 |
| `add_child(widget, row, column, row_span, column_span)` | 添加跨行跨列子组件 |

适用场景：

- 计算器按钮区
- 宫格面板
- 表单矩阵
- 仪表盘面板

示例：

```c++
auto keypad = Nandina::GridLayout::Create();
keypad->set_column_count(3)
      .set_gap(12.0f);

keypad->add_child(Nandina::Button::Create()->set_text("1"), 0, 0);
keypad->add_child(Nandina::Button::Create()->set_text("2"), 0, 1);
keypad->add_child(Nandina::Button::Create()->set_text("3"), 0, 2);
keypad->add_child(Nandina::Button::Create()->set_text("0"), 1, 0, 1, 3);
```

### 5.5 StackLayout

多个子组件叠放在同一块区域。

典型接口：

| 接口 | 作用 |
| :-- | :-- |
| `set_active_index(int)` | 切换当前显示子项 |
| `set_fill_mode(FillMode)` | 控制是否填满父区域 |
| `set_alignment(Align)` | 当不填满时控制对齐 |

适用场景：

- 页面切换
- loading / content / error 三态
- overlay、tooltip、badge

示例：

```c++
auto stack = Nandina::StackLayout::Create();
stack->add_child(Nandina::Label::Create()->set_text("Loading..."));
stack->add_child(Nandina::Widget::Create()->set_background(40, 44, 52));
stack->set_active_index(1);
```

### 5.6 Spacer

提供弹性空白，用于推动其它组件。

接口建议：

```c++
auto spacer = Nandina::Spacer::Create();
spacer->set_flex(1);
```

示例：

```c++
auto row = Nandina::HorizontalLayout::Create();
row->add_child(Nandina::Label::Create()->set_text("Left"));
row->add_child(Nandina::Spacer::Create()->set_flex(1));
row->add_child(Nandina::Button::Create()->set_text("Right"));
```

### 5.7 Expanded

表示“这个子组件要分配剩余空间”。

接口建议：

```c++
auto content = Nandina::Expanded::Create(
    Nandina::Widget::Create(),
    1
);
```

也可以通过 `LayoutParams().set_flex(1)` 替代。

适用场景：

- 页面主内容区
- 侧边栏 + 主区域
- 中央可伸缩区域

### 5.8 SizedBox

为子项提供固定尺寸约束。

接口建议：

| 接口 | 作用 |
| :-- | :-- |
| `set_width(float)` | 固定宽度 |
| `set_height(float)` | 固定高度 |
| `set_size(Size)` | 固定尺寸 |
| `child(widget)` | 设置唯一子项 |

示例：

```c++
auto title_bar = Nandina::SizedBox::Create();
title_bar->set_height(64.0f)
         .child(Nandina::Label::Create()->set_text("Nandina"));
```

### 5.9 Padding

提供内边距包裹能力。

接口建议：

```c++
auto padded = Nandina::Padding::Create();
padded->set_padding(24.0f)
      .child(Nandina::Label::Create()->set_text("Hello"));
```

如果后续统一 API，建议命名全部改为：

- `set_padding(float)`
- `set_padding(horizontal, vertical)`
- `set_padding(left, top, right, bottom)`

### 5.10 Center

用于让单个子组件在父区域内居中。

接口建议：

```c++
auto center = Nandina::Center::Create();
center->child(Nandina::Label::Create()->set_text("0"));
```

适用场景：

- 数字居中显示
- 空状态提示
- loading 提示

## 6. 布局系统推荐的使用方式

### 6.1 页面级用 VerticalLayout 组织大结构

大多数页面首先应该从纵向主结构开始：

```c++
auto page = Nandina::Widget::Create();
page->set_layout(
    Nandina::VerticalLayout::Create()
        ->set_padding(40.0f)
        .set_gap(24.0f)
);
```

常见结构：

- 标题栏
- 主内容区
- 底部操作区

### 6.2 局部区域再嵌套 HorizontalLayout 或 GridLayout

例如按钮组：

```c++
auto button_row = Nandina::HorizontalLayout::Create();
button_row->set_gap(16.0f)
          .set_align_items(Nandina::Align::Center)
          .set_justify_content(Nandina::JustifyContent::Center);
```

例如九宫格：

```c++
auto grid = Nandina::GridLayout::Create();
grid->set_column_count(3)
    .set_gap(10.0f);
```

### 6.3 对单个区域的特殊约束使用包装布局

例如：

- 用 `SizedBox` 固定标题栏高度
- 用 `Padding` 做局部留白
- 用 `Center` 做单元素居中
- 用 `Expanded` 表示内容占满剩余空间

这类组件本质上是“布局辅助组件”，应与主布局容器协作，而不是互相替代。

## 7. 理想用法示例

下面给出一个理想状态下的页面示例。

```c++
class DemoPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<DemoPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    Nandina::State<int> count_{0};
};

auto DemoPage::Create() -> std::unique_ptr<DemoPage> {
    auto self = std::make_unique<DemoPage>();
    self->initialize();
    return self;
}

auto DemoPage::build() -> Nandina::WidgetPtr {
    auto page = Nandina::Widget::Create();

    auto root = Nandina::VerticalLayout::Create();
    root->set_padding(40.0f)
        .set_gap(24.0f);
    page->set_layout(std::move(root));

    page->layout()->add_child(
        Nandina::SizedBox::Create()
            ->set_height(48.0f)
            .child(
                Nandina::Label::Create()
                    ->set_text("Nandina Counter")
                    .set_font(Nandina::Font::Pixels(24.0f).color("#c8c8ff"))
            )
    );

    page->layout()->add_child(
        Nandina::Expanded::Create(
            Nandina::Center::Create()->child(
                Nandina::Label::Create()
                    ->bind_text(count_, [](int value) {
                        return std::format("{}", value);
                    })
                    .set_font(Nandina::Font::Pixels(48.0f).color("#dcf0e8"))
            ),
            1
        )
    );

    page->layout()->add_child(
        Nandina::HorizontalLayout::Create()
            ->set_gap(16.0f)
            .set_justify_content(Nandina::JustifyContent::Center)
            .add_child(
                Nandina::Button::Create()
                    ->set_text("-1")
                    .on_click([this] {
                        count_.set(count_.get() - 1);
                    })
            )
            .add_child(
                Nandina::Button::Create()
                    ->set_text("+1")
                    .on_click([this] {
                        count_.set(count_.get() + 1);
                    })
            )
    );

    return page;
}
```

这段代码体现的理想特征是：

- 页面结构主要通过布局表达，而不是手算坐标。
- 子组件添加之后不需要手动推导所有权。
- 文本绑定通过高阶 API 完成，而不是手写 effect + 裸指针捕获。
- 对开发者而言，布局、样式、状态绑定三者的职责是清晰的。

## 8. 和当前实现的关系

当前代码库已经具备以下雏形：

- `Row`
- `Column`
- `Stack`
- `Spacer`
- `Expanded`
- `SizedBox`
- `Padding`
- `Center`

但当前实现还属于第一阶段原型，距离理想设计还有以下差距：

- 接口命名尚未统一
- `layout()` 触发机制还不够自动
- 嵌套布局的递归行为还不够稳定
- 缺少 GridLayout / FlowLayout / FormLayout 等更完整的布局组件
- 子组件布局参数系统还未建立
- 页面写法仍然会暴露部分所有权和生命周期细节

因此，这份文档更适合作为布局系统后续演进的目标说明，而不是当前 API 的精确参考。
## 本章演示了在理想情况下的layout设计

### Layout type

所有`Widget`子类都可以使用布局来管理它们的子类。`setLayout()` 函数将布局应用到 widget 上。
当以这种方式在部件上设置布局时，它将负责以下任务：

- 定位子部件
- 窗口的合理默认尺寸
- 窗口的合理最小尺寸
- 调整大小处理
- 内容变化时自动更新
    - 子部件的字体大小、文本或其他内容
    - 隐藏或显示子部件
    - 删除子窗口小部件

| layout class     | description |
|------------------|-------------|
| BoxLayout        | 水平或垂直排列子部件  |
| HorizontalLayout | 水平排列子部件     |
| VerticalLayout   | 垂直排列子部件     |
| GridLayout       | 在网格中排列子部件   |
| Spacer           | 提供组件间的透明间距  |

```c++
// main.cpp
import Nandina.Application;
import ApplicationWindow;

int main() {
    Nandina::NanApplication app;
    ApplicationWindow window;
    return window.exec();
}
```

```c++
// test_page.ixx
class DemoPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<DemoPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    Nandina::State<int> count_{0};
};

auto DemoPage::Create() -> std::unique_ptr<DemoPage> {
    auto self = std::make_unique<DemoPage>();
    self->initialize();
    return self;
}

auto DemoPage::build() -> Nandina::WidgetPtr {
    auto page = Nandina::Column::Create();
    page->set_spacing(20)
        .set_padding_top(15.0f)
        .set_padding_bottom(15.0f)
        .set_padding_left(20.0f)
        .set_padding_right(20.0f)
        .set_gap(24.0f);
    
    auto root_layout = Nandina::RowLayout::Create();
    root_layout->set_gap(20.0f);
            .set_alignment(Nandina::Alignment::Center);
            .set_justify_content(Nandina::JustifyContent::Center);
    
    page->set_layout(root_layout);
    
    
    auto button_group_layout = Nandina::GridLayout::Create();
    button_group_layout->set_column_count(3)
        .set_row_count(3)
        .set_gap(10.0f);
    
    auto label_group_layout = Nandina::ColumnLayout::Create();
    label_group_layout->set_gap(10.0f);
        .set_alignment(Nandina::Alignment::Center);
        .set_justify_content(Nandina::JustifyContent::Center);
    
    root_layout->add_child(button_group_layout);
    root_layout->add_child(label_group_layout);
    
    for (int i = 0; i < 9; ++i) {
        auto button = Nandina::Button::Create("Button " + std::to_string(i + 1));
        button_group_layout->add_child(button);
    }
    
    auto label1 = Nandina::Label::Create("Label 1");
    auto label2 = Nandina::Label::Create("Label 2");
    auto label3 = Nandina::Label::Create("Label 3");
    label_group_layout->add_child(label1);
                       .add_child(label2);
                       .add_child(label3);
    
    return page;
     
}
```

```c++
// application_window.ixx
export class ApplicationWindow final : public Nandina::WindowController {
protected:
    auto setup() -> void override;

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override;

    [[nodiscard]] auto title() const -> std::string_view override;

private:
    Nandina::Router router_;
};

void ApplicationWindow::setup() {
    set_background_color(35, 38, 52);

    // DemoPage via Router (demonstrates FlexLayout)
    router_.push<DemoPage>();
    auto view = Nandina::RouterView::Create(router_);
    view->set_bounds(0.0f, 0.0f,
                    static_cast<float>(window_width()),static_cast<float>(window_height()));
    set_content(std::move(view));
}

std::pair<int, int> ApplicationWindow::initial_size() const {
    return {640, 480};
}

std::string_view ApplicationWindow::title() const {
    return "Nandina — Counter [RenderLayer Demo]";
}
```