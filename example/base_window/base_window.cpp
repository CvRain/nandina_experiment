//
// Created by cvrain on 2026/8/10.
//

#include "base_window.hpp"

namespace nandina::example::base_window
{

    auto MainPage::build(widget::BuildContext& ui) -> widget::View {
        auto& button_string = ui.signal<std::string>("clicked me");

        auto button = ui.button(button_string);
        button.on_click([&]() { button_string.set("clicked!"); });

        const auto content = ui.center().child(button);

        return content.build();
    }
} // namespace nandina::example::base_window
