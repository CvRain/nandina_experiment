# NovaUI 设计文档（当前实现对齐版）

## 1. 项目目标

NovaUI 目标是在 C++ 桌面端提供接近前端框架的开发体验：

1. 组件可组合，页面可拆分。
2. API 语义化，支持链式调用与回调事件。
3. 架构清晰，避免宏系统与重型反射依赖。
4. 保留现代 UI 系统的扩展能力（主题、布局、动画、复合组件）。

---

## 2. 技术选型

| 层 | 选型 | 说明 |
| :-- | :-- | :-- |
| Shell | SDL3 | 窗口、事件、纹理呈现，跨平台稳定 |
| Render | ThorVG | 矢量绘制与软件后端，适合逐步演进 |
| Layout | Yoga（规划） | 后续接入 Flex 布局，当前先保留接口 |
| Build | CMake + Ninja | 使用 C++ modules 依赖扫描，规避 Meson 顺序问题 |

说明：当前仓库主线已切换 CMake，模块构建由 CMake 扫描管理。

---

## 3. 架构概览

采用保留模式组件树（Retained Mode）：

1. Page 层：用户自定义页面与窗口子类。
2. WindowController 层：生命周期与主循环控制。
3. Component/Widget 层：树结构、事件、状态。
4. Renderer 层：将组件树转为 ThorVG paint 并提交。

### 3.1 数据流

1. SDL 事件进入窗口。
2. 事件转换为内部 Event。
3. 组件树命中测试并分发。
4. 组件状态变化触发重绘。
5. 渲染器将当前组件树绘制到窗口。

### 3.2 组件组合思想

核心原则：复杂组件由基础组件组合而成，而不是单体巨类。

例如 Button：

1. FocusComponent：焦点/高亮层。
2. RectangleComponent：几何背景层。
3. Button 自身：输入事件与组合行为控制。

---

## 4. 当前模块说明

### 4.1 Core 模块

文件：[src/core/Core.ixx](src/core/Core.ixx)

包含：

1. Event / MouseButton
2. Signal / Connection
3. Widget（基础树与事件能力）
4. Component（可继承业务基类）
5. RectangleComponent / FocusComponent / Button（组合示例）

### 4.2 Window 模块

文件：[src/core/Window.ixx](src/core/Window.ixx)

包含：

1. NanWindow：底层窗口与渲染执行器。
2. WindowController：可继承窗口控制器（类似 Qt 风格）。

WindowController 对外约定：

1. `title()`：窗口标题。
2. `initial_size()`：初始大小。
3. `build_root()`：构建页面根组件。
4. `on_start()/on_frame()/on_shutdown()`：生命周期扩展点。

### 4.3 几何类型模块

文件：

1. [src/types/Position.ixx](src/types/Position.ixx)
2. [src/types/Size.ixx](src/types/Size.ixx)
3. [src/types/Rect.ixx](src/types/Rect.ixx)

说明：这些类型作为后续布局、命中测试和样式系统的公共基石。

---

## 5. 设计原则

1. 用户代码不直接操作底层窗口细节。
2. 组件优先组合而非继承链膨胀。
3. 事件系统保持类型安全与低心智负担。
4. 先打通最小闭环，再逐步提升样式与布局能力。

---

## 6. 页面开发建议范式

建议使用“页面子类 + build_root”方式组织业务 UI：

1. 每个 Page 自己管理组件树。
2. WindowController 管理执行循环。
3. 回调逻辑写在页面或组件内。

示例（抽象）：

```cpp
class SettingsWindow : public Nandina::WindowController {
protected:
    auto build_root() -> std::unique_ptr<Nandina::Widget> override {
        auto page = std::make_unique<Nandina::Component>();
        auto save = Nandina::Button::Create();
        save->set_bounds(560, 500, 220, 56);
        page->add_child(std::move(save));
        return page;
    }
};
```

---

## 7. 后续演进方向

### 7.1 布局系统

1. 先实现轻量容器（HStack/VStack/FlexContainer）占位。
2. 保持 API 稳定后接入 Yoga。

### 7.2 样式系统

1. Theme Tokens（颜色、间距、圆角、字体）。
2. Variant Recipe（Primary/Ghost/Destructive）。
3. 状态样式（hover/pressed/focused/disabled）。

### 7.3 文本系统

1. Button 文本可视化。
2. 后续接入字体加载与排版。

### 7.4 组件库扩展

1. Input、Checkbox、Switch。
2. Dialog、Dropdown、Toast。

---

## 8. 已知约束

1. 当前 Button 组合已具备交互反馈，但文本绘制尚未实现。
2. 当前布局主要依赖绝对坐标，Flex 仍在后续阶段。
3. 主题系统尚未启用全局 Token 解析流程。

---

## 9. 里程碑判定标准

MVP 阶段达成标准：

1. 可继承窗口模型稳定运行。
2. 组件树可构建、可响应点击。
3. 组合组件（Button）具备可见状态变化。
4. CMake 构建稳定可复现。

