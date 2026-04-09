module;

#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module CounterPage;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;
import Nandina.Types;

export // ── CounterPage ───────────────────────────────────────────────────────────────
// Demonstrates State + Effect reactive binding inside a Page.
// Pressing "+1" / "-1" updates a count_, which the Effect propagates to the label.
// Uses FlexLayout (Expanded + Spacer) to demonstrate the two-pass layout algorithm.
// ─────────────────────────────────────────────────────────────────────────────
class CounterPage : public Nandina::Page {
public:
    static auto Create() -> std::unique_ptr<CounterPage>;

    auto build() -> Nandina::WidgetPtr override;

private:
    Nandina::State<int> count_{0};
};

auto CounterPage::Create() -> std::unique_ptr<CounterPage> {
    auto self = std::make_unique<CounterPage>();
    self->initialize();
    return self;
}

auto CounterPage::build() -> Nandina::WidgetPtr {
    constexpr float page_width = 640.0f;
    constexpr float page_height = 480.0f;
    constexpr float title_x = 40.0f;
    constexpr float title_y = 24.0f;
    constexpr float title_width = 560.0f;
    constexpr float title_height = 48.0f;
    constexpr float number_width = 240.0f;
    constexpr float number_height = 64.0f;
    constexpr float number_x = (page_width - number_width) * 0.5f;
    constexpr float number_y = (page_height - number_height) * 0.5f;
    constexpr float button_width = 120.0f;
    constexpr float button_height = 60.0f;
    constexpr float button_gap = 16.0f;
    constexpr float button_row_width = button_width * 2.0f + button_gap;
    constexpr float button_row_x = (page_width - button_row_width) * 0.5f;
    constexpr float button_row_y = number_y + number_height + 24.0f;

    auto root = std::make_unique<Nandina::FreeWidget>();
    root->move_to(0.0f, 0.0f).resize(page_width, page_height);
    root->set_background(0, 0, 0, 0);

    auto title_label = Nandina::Label::Create();
    title_label->set_background(0, 0, 0, 0);
    title_label->set_bounds(title_x, title_y, title_width, title_height);
    title_label->font_size(24.0f).text_color(200, 200, 255);
    title_label->text("Nandina Counter");
    root->add_child(std::move(title_label));

    auto number_label = Nandina::Label::Create();
    number_label->set_background(0, 0, 0, 0);
    number_label->set_bounds(number_x, number_y, number_width, number_height);
    number_label->font_size(48.0f).text_color(220, 240, 232);
    number_label->text("0");
    auto *number_label_ptr = number_label.get();
    root->add_child(std::move(number_label));

    this->effect([this, number_label_ptr]() {
        number_label_ptr->text(std::format("{}", count_()));
    });


    auto decrease_button = Nandina::Button::Create();
    decrease_button->set_bounds(button_row_x, button_row_y, button_width, button_height);
    decrease_button->text("-1");
    decrease_button->set_background(234, 153, 156);
    decrease_button->on_click([this]() {
        count_.set(count_() - 1);
    });

    root->add_child(std::move(decrease_button));

    auto increase_button = Nandina::Button::Create();
    increase_button->set_bounds(button_row_x + button_width + button_gap,
                                button_row_y,
                                button_width,
                                button_height);
    increase_button->text("+1");
    increase_button->set_background(234, 153, 156);
    increase_button->on_click([this]() {
        count_.set(count_() + 1);
    });


    root->add_child(std::move(increase_button));

    return root;
}
