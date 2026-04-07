module;

#include <format>
#include <memory>
#include <string>
#include <string_view>
#include <utility>

export module ApplicationWindow;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;
import Nandina.Types;

// ── CounterPage ───────────────────────────────────────────────────────────────
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
    auto* number_label_ptr = number_label.get();
    root->add_child(std::move(number_label));

    this->effect([this, number_label_ptr]() {
        number_label_ptr->text(std::format("{}", count_.get()));
    });


    auto decrease_button = Nandina::Button::Create();
    decrease_button->set_bounds(button_row_x, button_row_y, button_width, button_height);
    decrease_button->text("-1");
    decrease_button->set_background(234, 153, 156);
    decrease_button->on_click([this]() {
        count_.set(count_.get() - 1);
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
        count_.set(count_.get() + 1);
    });


    root->add_child(std::move(increase_button));

    return root;
}

// ── ApplicationWindow ─────────────────────────────────────────────────────────
// Uses the multi-layer RenderLayer architecture:
//   Layer 0 (World)   — dark background FreeWidget
//   Layer 1 (UI)      — RouterView with CounterPage
//   Layer 2 (Overlay) — semi-transparent badge in the bottom-right corner
// ─────────────────────────────────────────────────────────────────────────────
export class ApplicationWindow final : public Nandina::WindowController {
protected:
    auto setup() -> void override;

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override;

    [[nodiscard]] auto title() const -> std::string_view override;

private:
    Nandina::Router router_;
};

void ApplicationWindow::setup() {
    constexpr float badge_width = 82.0f;
    constexpr float badge_height = 20.0f;
    constexpr float badge_margin_x = 8.0f;
    constexpr float badge_margin_y = 8.0f;

    set_background_color(35, 38, 52);

    // CounterPage via Router (demonstrates FlexLayout)
    router_.push<CounterPage>();
    auto view = Nandina::RouterView::Create(router_);
    view->set_bounds(0.0f, 0.0f,
                     static_cast<float>(window_width()),
                     static_cast<float>(window_height()));
    set_content(std::move(view));

    // Version badge in the bottom-right corner (FreeWidget, floats above UI)
    auto badge = std::make_unique<Nandina::FreeWidget>();
    badge->layer(2);
    badge->move_to(static_cast<float>(window_width()) - badge_width - badge_margin_x,
                   static_cast<float>(window_height()) - badge_height - badge_margin_y)
            .resize(badge_width, badge_height)
            .set_background(231, 130, 132, 255);

    auto badge_label = Nandina::Label::Create();
    badge_label->layer(2);
    badge_label->set_background(0, 0, 0, 0);
    badge_label->text("v0.0.1-alpha");
    badge_label->font_size(12.0f).text_color(220, 224, 232);

    const auto &badge_position = badge->position();
    badge_label->set_position({badge_position.x() + 2.0f, badge_position.y() + 8.0f});

    badge->add_child(std::move(badge_label));

    add_child(std::move(badge));
}

std::pair<int, int> ApplicationWindow::initial_size() const {
    return {640, 480};
}

std::string_view ApplicationWindow::title() const {
    return "Nandina — Counter [RenderLayer Demo]";
}
