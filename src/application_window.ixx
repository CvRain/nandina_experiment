module;

#include <memory>
#include <string_view>
#include <utility>

export module ApplicationWindow;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;

// ApplicationWindow — 继承 WindowController，填充 exec() 所需的三个虚函数。
// 等同于 Qt 的 QMainWindow 子类，作为编写 UI 测试代码的入口。
export class ApplicationWindow final : public Nandina::WindowController {
protected:
    auto build_root() -> std::unique_ptr<Nandina::Widget> override;

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override;

    [[nodiscard]] auto title() const -> std::string_view override;
};

// ── build_root ────────────────────────────────────────────────────────────────
// 使用 Column 布局（M2 集成测试：Row/Column 替换绝对坐标）
auto ApplicationWindow::build_root() -> std::unique_ptr<Nandina::Widget> {
    auto root = Nandina::Column::Create();
    root->set_bounds(0, 0, 640, 480)
         .set_background(30, 30, 30);
    root->gap(12.0f).padding(24.0f);

    // Label
    auto label = Nandina::Label::Create();
    label->set_bounds(0, 0, 0, 30); // width 由 Column::layout() 计算，height 预设
    label->text("Hello, Nandina!").font_size(20.0f).text_color(0, 0, 0);
    root->add(std::move(label));

    // Button
    auto btn = Nandina::Button::Create();
    btn->set_bounds(0, 0, 0, 40);
    btn->text("Click Me");
    root->add(std::move(btn));

    root->layout(); // Column 根据 gap/padding 计算子组件位置
    return root;
}

// ── initial_size ──────────────────────────────────────────────────────────────
auto ApplicationWindow::initial_size() const -> std::pair<int, int> {
    return {640, 480};
}

// ── title ─────────────────────────────────────────────────────────────────────
auto ApplicationWindow::title() const -> std::string_view {
    return "Nandina — Composed UI";
}
