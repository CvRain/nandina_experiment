//
// widget/text_field — semantic single-line text input shell.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TEXT_FIELD_HPP
#define NANDINA_EXPERIMENT_WIDGET_TEXT_FIELD_HPP

#include "../theme/theme.hpp"
#include "primitives/editable_text.hpp"
#include "primitives/surface.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nandina::widget
{

    class TextField: public scene::NanControl {
    public:
        explicit TextField(
            std::string value = {},
            std::string placeholder = {},
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::string value = {},
            std::string placeholder = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<TextField>;

        void set_value(std::string value);
        [[nodiscard]] auto value() const -> std::string_view;

        void set_placeholder(std::string placeholder);
        [[nodiscard]] auto placeholder() const -> std::string_view;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;

        void set_on_change(std::function<void(std::string_view)> callback);

        [[nodiscard]] auto editable_text() -> primitives::EditableText&;
        [[nodiscard]] auto editable_text() const -> const primitives::EditableText&;

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints) -> foundation::NanSize override;

    private:
        void apply_theme();
        [[nodiscard]] auto content_constraints(scene::LayoutConstraints constraints) const
            -> scene::LayoutConstraints;
        [[nodiscard]] auto content_origin(foundation::NanRect world) const -> foundation::NanPoint;

        primitives::Surface surface_;
        primitives::EditableText edit_;
        primitives::Text placeholder_;
        theme::NanTheme theme_;
        float padding_x_ = 0.0F;
        float height_ = 0.0F;
        std::function<void(std::string_view)> on_change_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TEXT_FIELD_HPP
