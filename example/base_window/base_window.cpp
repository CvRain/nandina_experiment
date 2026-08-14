//
// Created by cvrain on 2026/8/10.
//

#include "base_window.hpp"

#include "widget/controls.hpp"

#include <algorithm>

namespace nandina::example::base_window
{

    auto MainPage::build(widget::BuildContext& ui) -> widget::View {
        auto& button_string = ui.signal<std::string>("advance +25%");
        auto& progress = ui.signal<float>(0.0F);

        auto button = ui.make<widget::Button>(button_string);
        button.on_click([&]() {
            progress.set(std::clamp(progress.peek() + 0.25F, 0.0F, 1.0F));
        })
        .width(scene::percent(55))
        .height(scene::percent(15))
        .font_size(scene::percent(45));
        // 可选：百分比尺寸约束上限（若同时设置 width(55%)，实际宽度会被截断到 40%）。
        // button.max_width(scene::percent(40));

        auto bar = ui.make<widget::ProgressBar>(progress).width(scene::percent(70));

        const auto content = ui.column()
                                 .gap(24.0F)
                                 .cross_alignment(widget::LayoutAlignment::center)
                                 .children(button, bar)
                                 .width(scene::percent(100))
                                 .height(scene::percent(100));

        return content.build();
    }
} // namespace nandina::example::base_window
