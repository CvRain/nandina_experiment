//
// widget/text_field — semantic single-line text input shell.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_TEXT_FIELD_HPP
#define NANDINA_EXPERIMENT_WIDGET_TEXT_FIELD_HPP

#include "../theme/theme.hpp"
#include "../theme/text_field_style.hpp"
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
        void set_on_submit(std::function<void(std::string_view)> callback);

        void set_read_only(bool value);
        [[nodiscard]] auto read_only() const -> bool;
        void set_disabled(bool value);
        [[nodiscard]] auto disabled() const -> bool;
        void set_invalid(bool value);
        [[nodiscard]] auto invalid() const -> bool;
        [[nodiscard]] auto visual_state() const -> theme::TextFieldVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::TextFieldStyle;

        [[nodiscard]] auto editable_text() -> primitives::EditableText&;
        [[nodiscard]] auto editable_text() const -> const primitives::EditableText&;
        [[nodiscard]] auto placeholder_text() -> primitives::Text&;
        [[nodiscard]] auto placeholder_text() const -> const primitives::Text&;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        void on_theme_context_removed() override;
        void set_font(text::FontRequest request);
        void set_font_family(resource::ResourceKey family);
        void set_font_weight(int weight);
        void set_font_slant(text::FontSlant slant);

        [[nodiscard]] auto is_focusable() const -> bool override;
        auto on_input(scene::InputEvent& event) -> bool override;
        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;

    private:
        void apply_theme();
        void place_caret(foundation::NanPoint screen_point, bool extend);
        void update_scroll(float viewport_width);
        [[nodiscard]] auto content_constraints(scene::LayoutConstraints constraints) const
            -> scene::LayoutConstraints;
        [[nodiscard]] auto line_origin(
            foundation::NanRect world,
            const primitives::TextLayoutResult& layout,
            float x
        ) const -> foundation::NanPoint;

        primitives::Surface surface_;
        primitives::EditableText edit_;
        primitives::Text placeholder_;
        theme::NanTheme theme_;
        const theme::ThemeManager* theme_manager_ = nullptr;
        bool theme_explicit_ = false;
        bool font_explicit_ = false;
        float padding_x_ = 0.0F;
        float height_ = 0.0F;
        float scroll_x_ = 0.0F;
        bool focused_ = false;
        bool dragging_ = false;
        bool read_only_ = false;
        bool disabled_ = false;
        bool invalid_ = false;
        std::function<void(std::string_view)> on_change_;
        std::function<void(std::string_view)> on_submit_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_TEXT_FIELD_HPP
