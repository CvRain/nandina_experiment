//
// Created by cvrain on 2026/8/10.
//

#include "base_window.hpp"

#include "widget/controls.hpp"

namespace nandina::example::base_window
{

    auto MainPage::build(widget::BuildContext& ui) -> widget::View {
        auto& button_string = ui.signal<std::string>("clicked me");

        auto button = ui.make<widget::Button>(button_string);
        button.on_click([&]() { button_string.set("clicked!"); })
        .width(scene::percent(55))
        .height(scene::percent(15))
        .font_size(scene::percent(45));
        // 可选：百分比尺寸约束上限（若同时设置 width(55%)，实际宽度会被截断到 40%）。
        // button.max_width(scene::percent(40));

        const auto content =
            ui.center().child(button).width(scene::percent(100)).height(scene::percent(100));

        return content.build();
    }
} // namespace nandina::example::base_window
