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
        col->set_background(30, 30, 30);
        col->gap(16.0f).padding(40.0f);

        // Label — displays the current count via a reactive Effect
        auto label = Nandina::Label::Create();
        label->set_size(Nandina::Size::fixed(400.0f, 48.0f));
        label->font_size(32.0f).text_color(255, 255, 255);

        effect([this, lbl = label.get()] {
            lbl->text(std::format("Count: {}", count_.get()));
        });

        // Button row
        auto row = Nandina::Row::Create();
        row->set_size(Nandina::Size::fixed(300.0f, 48.0f));
        row->gap(12.0f);

        auto dec = Nandina::Button::Create();
        dec->set_size(Nandina::Size::fixed(120.0f, 48.0f));
        dec->text("-1");
        dec->on_click([this] { count_.set(count_.get() - 1); });

        auto inc = Nandina::Button::Create();
        inc->set_size(Nandina::Size::fixed(120.0f, 48.0f));
        inc->text("+1");
        inc->on_click([this] { count_.set(count_.get() + 1); });

        row->add(std::move(dec)).add(std::move(inc));
        row->layout();

        col->add(std::move(label)).add(std::move(row));
        col->layout();

        return col;
    }

    auto on_enter() -> void override {}

private:
    Nandina::State<int> count_{ 0 };
};


// ── ApplicationWindow ─────────────────────────────────────────────────────────
// Initialises the Router, pushes CounterPage as the first screen, and places a
// RouterView that fills the entire window.
// ─────────────────────────────────────────────────────────────────────────────
export class ApplicationWindow final : public Nandina::WindowController {
protected:
    auto build_root() -> std::unique_ptr<Nandina::Widget> override {
        router_.push<CounterPage>();

        auto view = Nandina::RouterView::Create(router_);
        view->set_bounds(0.0f, 0.0f, 640.0f, 480.0f);
        return view;
    }

    [[nodiscard]] auto initial_size() const -> std::pair<int, int> override {
        return {640, 480};
    }

    [[nodiscard]] auto title() const -> std::string_view override {
        return "Nandina — Counter";
    }

private:
    Nandina::Router router_;
};

