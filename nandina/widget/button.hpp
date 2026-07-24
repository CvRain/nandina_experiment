//
// widget/button — first semantic control built from tone/treatment tokens.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP

#include "../theme/button_style.hpp"
#include "primitives/pressable.hpp"
#include "primitives/text.hpp"

#include <memory>
#include <string>
#include <string_view>

namespace nandina::widget
{

    class Button: public primitives::Pressable {
    public:
        explicit Button(std::string text, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto
        create(std::string text, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Button>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;
        [[nodiscard]] auto text_node() -> primitives::Text&;
        [[nodiscard]] auto text_node() const -> const primitives::Text&;

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

        void set_text_overflow(primitives::TextOverflow overflow);
        [[nodiscard]] auto text_overflow() const -> primitives::TextOverflow;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;

        void set_tone(theme::ButtonTone tone);
        [[nodiscard]] auto tone() const -> theme::ButtonTone;

        void set_treatment(theme::ButtonTreatment treatment);
        [[nodiscard]] auto treatment() const -> theme::ButtonTreatment;

        void set_button_size(theme::ButtonSize size);
        [[nodiscard]] auto button_size() const -> theme::ButtonSize;

        [[nodiscard]] auto visual_state() const -> theme::ButtonVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::ButtonStyle;

        auto on_draw(render::DrawContext& ctx) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        void apply_metrics();
        void apply_text_style(theme::ButtonVisualState state);

        primitives::Text text_;
        theme::NanTheme theme_;
        const theme::ThemeManager* theme_manager_ = nullptr;
        bool theme_explicit_ = false;
        bool font_explicit_ = false;
        theme::ButtonTone tone_ = theme::ButtonTone::primary;
        theme::ButtonTreatment treatment_ = theme::ButtonTreatment::filled;
        theme::ButtonSize size_ = theme::ButtonSize::medium;
        primitives::TextOverflow text_overflow_ = primitives::TextOverflow::ellipsis;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
