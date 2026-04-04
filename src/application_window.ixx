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
    static auto Create() -> std::unique_ptr<CounterPage> {
        auto self = std::unique_ptr<CounterPage>(new CounterPage());
        self->initialize();
        return self;
    }

    auto build() -> Nandina::WidgetPtr override {
        auto col = Nandina::Column::Create();
        col->set_bounds(0.0f, 0.0f, 640.0f, 480.0f);
        col->set_background(0, 0, 0, 0);  // transparent — Layer 0 background shows through
        col->gap(16.0f).padding(40.0f);

        // ── Fixed-height title ─────────────────────────────────────────────
        auto title_box = Nandina::SizedBox::Create();
        title_box->height(48.0f);
        title_box->set_background(0, 0, 0, 0);
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
        });
        count_center->child(std::move(count_label));
        center_exp->child(std::move(count_center));

        // ── Elastic spacer (1 flex share) ──────────────────────────────────
        auto spacer = Nandina::Spacer::Create(1);

        // ── Button row (fixed height) ──────────────────────────────────────
        auto btn_row = Nandina::Row::Create();
        btn_row->set_size(Nandina::Size::fixed(560.0f, 52.0f));
        btn_row->gap(12.0f).padding(0.0f);
        btn_row->set_background(0, 0, 0, 0);

        auto left_spacer = Nandina::Spacer::Create(1);

        auto dec = Nandina::Button::Create();
        dec->set_size(Nandina::Size::fixed(120.0f, 52.0f));
        dec->text("-1");
        dec->on_click([this] { count_.set(count_.get() - 1); });

        auto inc = Nandina::Button::Create();
        inc->set_size(Nandina::Size::fixed(120.0f, 52.0f));
        inc->text("+1");
        inc->on_click([this] { count_.set(count_.get() + 1); });

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

private:
    Nandina::State<int> count_{ 0 };
};


// ── ApplicationWindow ─────────────────────────────────────────────────────────
// Uses the multi-layer RenderLayer architecture:
//   Layer 0 (World)   — dark background FreeWidget
//   Layer 1 (UI)      — RouterView with CounterPage
//   Layer 2 (Overlay) — semi-transparent badge in the bottom-right corner
// ─────────────────────────────────────────────────────────────────────────────
export class ApplicationWindow final : public Nandina::WindowController {
protected:
    auto setup_layers(std::array<Nandina::RenderLayer, 16>& layers) -> void override {
        constexpr float W = 640.0f, H = 480.0f;

        // ── Layer 0: World ─────────────────────────────────────────────────
        // Deep dark background (FreeWidget, absolute positioning, not in Flow layout)
        auto bg = std::make_unique<Nandina::FreeWidget>();
        bg->move_to(0.0f, 0.0f).resize(W, H);
        bg->set_background(15, 15, 25);  // deep blue-black
        layers[0].root = std::move(bg);

        // ── Layer 1: UI ────────────────────────────────────────────────────
        // CounterPage via Router (demonstrates FlexLayout)
        router_.push<CounterPage>();
        auto view = Nandina::RouterView::Create(router_);
        view->set_bounds(0.0f, 0.0f, W, H);
        layers[1].root = std::move(view);

        // ── Layer 2: Overlay ───────────────────────────────────────────────
        // Version badge in the bottom-right corner (FreeWidget, floats above UI)
        auto badge = std::make_unique<Nandina::FreeWidget>();
        badge->move_to(W - 90.0f, H - 28.0f).resize(82.0f, 20.0f);
        badge->set_background(30, 30, 60, 180);  // semi-transparent deep blue
        layers[2].root = std::move(badge);
    }

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override {
        return {640, 480};
    }

    [[nodiscard]] auto title() const -> std::string_view override {
        return "Nandina — Counter [RenderLayer Demo]";
    }

private:
    Nandina::Router router_;
};

