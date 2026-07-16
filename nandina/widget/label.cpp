//
// widget/label — semantic text control with optional reactive text binding.
//

#include "label.hpp"

#include <utility>

namespace nandina::widget
{

    Label::Label(reactive::Graph& graph, std::string text, theme::NanTheme theme):
        primitives::Text(std::move(text)),
        scope_(graph) {
        set_color(theme.palette.on_surface);
        set_font_size(theme.tokens.typography.label_lg);
    }

    auto Label::create(reactive::Graph& graph, std::string text, theme::NanTheme theme)
        -> std::shared_ptr<Label> {
        return std::make_shared<Label>(graph, std::move(text), theme);
    }

    void Label::activate_binding() {
        scope_.clear();
        if (binding_) {
            binding_(scope_, *this);
        }
    }

    void Label::on_ready() {
        primitives::Text::on_ready();
        activate_binding();
    }

    void Label::on_exit_tree() {
        scope_.clear();
        binding_ = {};
        primitives::Text::on_exit_tree();
    }

} // namespace nandina::widget
