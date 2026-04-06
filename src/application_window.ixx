module;

#include <format>
#include <memory>
#include <print>
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
    auto col = Nandina::Column::Create();
    col->set_bounds(0.0f, 0.0f, 640.0f, 480.0f);
    col->set_background(0, 0, 0, 0); // transparent — Layer 0 background shows through
    col->gap(16.0f).padding(40.0f);

    // ── Fixed-height title ─────────────────────────────────────────────
    auto title_box = Nandina::SizedBox::Create();
    title_box->height(48.0f).set_background(0, 0, 0, 0);

    auto title_label = Nandina::Label::Create();
    title_label->set_size(Nandina::Size::fixed(560.0f, 48.0f));
    title_label->font_size(24.0f).text_color(200, 200, 255);
    title_label->text("Nandina Counter");
    title_box->child(std::move(title_label));

    // ── Expanded center area (3 flex shares) ───────────────────────────
    auto center_exp = Nandina::Expanded::Create(3);

    auto count_center = Nandina::Center::Create();
    count_center->set_background(0, 0, 0, 0);

    auto count_label = Nandina::Label::Create();
    count_label->set_size(Nandina::Size::fixed(400.0f, 64.0f));
    count_label->font_size(48.0f).text_color(255, 255, 255);

    effect([this, lbl = count_label.get()] {
        lbl->text(std::format("Count: {}", count_.get()));
        std::printf("Count: %d\n", count_.get());
    });

    count_center->child(std::move(count_label));
    center_exp->child(std::move(count_center));

    // ── Elastic spacer (1 flex share) ──────────────────────────────────
    auto spacer = Nandina::Spacer::Create(1);

    // ── Button row (fixed height) ──────────────────────────────────────
    auto btn_row = Nandina::Row::Create();
    btn_row->set_size(Nandina::Size::fixed(560.0f, 52.0f));
    btn_row->gap(12.0f).padding(15.0f);
    btn_row->set_background(0, 0, 0, 0);

    auto left_spacer = Nandina::Spacer::Create(1);

    auto dec = Nandina::Button::Create();
    dec->set_size(Nandina::Size::fixed(120.0f, 60.0f));
    dec->text("-1");
    dec->set_background(234, 153, 156);
    auto *dec_button = dec.get();
    dec->on_click([this, dec_button] {
        count_.set(count_.get() - 1);
        std::println("decrease button clicked!");
    });

    auto inc = Nandina::Button::Create();
    inc->set_size(Nandina::Size::fixed(120.0f, 60.0f));
    inc->text("+1");
    inc->set_background(234, 153, 156);
    inc->on_click([this] {
        count_.set(count_.get() + 1);
        std::println("increase button clicked!");
    });

    auto right_spacer = Nandina::Spacer::Create(1);

    btn_row->add(std::move(left_spacer))
            .add(std::move(dec))
            .add(std::move(inc))
            .add(std::move(right_spacer));
    btn_row->layout();

    // ── Fixed-height version label ─────────────────────────────────────
    auto ver_box = Nandina::SizedBox::Create();
    ver_box->height(24.0f);
    ver_box->set_background(0, 0, 0, 0);
    auto ver_label = Nandina::Label::Create();
    ver_label->set_size(Nandina::Size::fixed(560.0f, 24.0f));
    ver_label->font_size(12.0f).text_color(100, 100, 140);
    ver_label->text("Nandina Experiment v0.3");
    ver_box->child(std::move(ver_label));

    col->add(std::move(title_box))
            .add(std::move(center_exp))
            .add(std::move(spacer))
            .add(std::move(btn_row))
            .add(std::move(ver_box));
    col->layout();

    return col;
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
    add_child(std::move(badge));
}

std::pair<int, int> ApplicationWindow::initial_size() const {
    return {640, 480};
}

std::string_view ApplicationWindow::title() const {
    return "Nandina — Counter [RenderLayer Demo]";
}
