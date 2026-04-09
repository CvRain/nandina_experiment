module;

#include <format>
#include <functional>
#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

export module ApplicationWindow;

import Nandina.Core;
import Nandina.Window;
import Nandina.Layout;
import Nandina.Components.Label;
import Nandina.Types;

import DemoPage;
import CounterPage;


// ── ApplicationWindow ─────────────────────────────────────────────────────────
// Uses the multi-layer RenderLayer architecture:
//   Layer 0 (World)   — dark background FreeWidget
//   Layer 1 (UI)      — RouterView with DemoPage (showcasing new primitives)
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
    set_background_color(35, 38, 52);

    router_.push<DemoPage>();
    auto view = Nandina::RouterView::Create(router_);
    view->set_bounds(0.0f, 0.0f,
                     static_cast<float>(window_width()),
                     static_cast<float>(window_height()));
    set_content(std::move(view));
}

std::pair<int, int> ApplicationWindow::initial_size() const {
    return {640, 480};
}

std::string_view ApplicationWindow::title() const {
    return "Nandina — Prop / StateList / Signal Demo";
}
