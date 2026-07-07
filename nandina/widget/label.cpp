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

    void Label::bind_text(reactive::Signal<std::string>& source) {
        signal_text_ = &source;
        computed_text_ = nullptr;
    }

    void Label::bind_text(reactive::Computed<std::string>& source) {
        computed_text_ = &source;
        signal_text_ = nullptr;
    }

    void Label::on_ready() {
        primitives::Text::on_ready();
        if (signal_text_ != nullptr) {
            scope_.add([this] { set_text(signal_text_->get()); });
        }
        if (computed_text_ != nullptr) {
            scope_.add([this] { set_text(computed_text_->get()); });
        }
    }

    void Label::on_exit_tree() {
        scope_.clear();
        primitives::Text::on_exit_tree();
    }

} // namespace nandina::widget
