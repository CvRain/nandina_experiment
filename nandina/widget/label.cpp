//
// widget/label — semantic text control with optional reactive text binding.
//

#include "label.hpp"
#include "../theme/theme_manager.hpp"

#include <utility>

namespace nandina::widget
{

    Label::Label(reactive::Graph& graph, std::string text, theme::NanTheme theme):
        primitives::Text(std::move(text)),
        scope_(graph),
        theme_(std::move(theme)) {
        apply_component_color(theme_.palette.on_surface);
        apply_component_font_size(theme_.tokens.typography.label_lg);
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

    void Label::on_theme_changed(const theme::ThemeManager& manager) {
        theme_ = manager.theme();
        if (!resolved_style_context().text_color_from_context) {
            apply_component_color(theme_.palette.on_surface);
        }
        if (!resolved_style_context().font_size_from_context) {
            apply_component_font_size(theme_.tokens.typography.label_lg);
        }
    }

    void Label::on_style_context_changed(const theme::ResolvedStyleContext& context) {
        primitives::Text::on_style_context_changed(context);
        if (!context.text_color_from_context) {
            apply_component_color(theme_.palette.on_surface);
        }
        if (!context.font_size_from_context) {
            apply_component_font_size(theme_.tokens.typography.label_lg);
        }
    }

} // namespace nandina::widget
