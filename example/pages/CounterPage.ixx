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
import Nandina.Layout;
import Nandina.Components.Label;

export // ── CounterPage ───────────────────────────────────────────────────────────────
// Demonstrates a reactive counter page using Label::bind_text and layout containers.
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
    constexpr float page_padding = 32.0f;
    constexpr float inner_width = page_width - page_padding * 2.0f;
    constexpr float title_width = 320.0f;
    constexpr float number_width = 240.0f;
    constexpr float number_height = 64.0f;
    constexpr float button_width = 120.0f;
    constexpr float button_height = 60.0f;
    constexpr float button_gap = 16.0f;
    constexpr float content_gap = 24.0f;

    auto root = Nandina::Column::Create();
    root->set_bounds(0.0f, 0.0f, page_width, page_height);
    root->set_background(28, 32, 46, 255);
    root->padding(page_padding).gap(content_gap).align_items(Nandina::Align::stretch);

    auto title_label = Nandina::Label::Create();
    title_label->set_background(0, 0, 0, 0);
    title_label->set_bounds(0.0f, 0.0f, title_width, 48.0f);
    title_label->font_size(24.0f).text_color(200, 200, 255);
    title_label->text("Counter Page");

    auto title_row = Nandina::Row::Create();
    title_row->justify_content(Nandina::Align::center).align_items(Nandina::Align::center);
    title_row->add(std::move(title_label));

    auto title_box = Nandina::SizedBox::Create();
    title_box->width(inner_width).height(48.0f).child(std::move(title_row));
    root->add(std::move(title_box));

    root->add(Nandina::Spacer::Create(1));

    auto number_label = Nandina::Label::Create();
    number_label->set_background(0, 0, 0, 0);
    number_label->set_bounds(0.0f, 0.0f, number_width, number_height);
    number_label->font_size(48.0f).text_color(220, 240, 232);
    number_label->bind_text(count_.as_read_only(), [](int value) {
        return std::format("{}", value);
    });

    auto number_row = Nandina::Row::Create();
    number_row->justify_content(Nandina::Align::center).align_items(Nandina::Align::center);
    number_row->add(std::move(number_label));

    auto number_box = Nandina::SizedBox::Create();
    number_box->width(inner_width).height(96.0f).child(std::move(number_row));
    root->add(std::move(number_box));

    auto decrease_button = Nandina::Button::Create();
    decrease_button->set_bounds(0.0f, 0.0f, button_width, button_height);
    decrease_button->text("-1");
    decrease_button->set_background(234, 153, 156);
    decrease_button->on_click([this]() {
        count_.set(count_() - 1);
    });

    auto increase_button = Nandina::Button::Create();
    increase_button->set_bounds(0.0f, 0.0f, button_width, button_height);
    increase_button->text("+1");
    increase_button->set_background(122, 162, 247);
    increase_button->on_click([this]() {
        count_.set(count_() + 1);
    });

    auto button_row = Nandina::Row::Create();
    button_row->gap(button_gap)
        .justify_content(Nandina::Align::center)
        .align_items(Nandina::Align::center);
    button_row->add(std::move(decrease_button));
    button_row->add(std::move(increase_button));

    auto button_box = Nandina::SizedBox::Create();
    button_box->width(inner_width).height(button_height).child(std::move(button_row));
    root->add(std::move(button_box));

    root->add(Nandina::Spacer::Create(2));

    return root;
}
