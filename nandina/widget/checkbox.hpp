//
// widget/checkbox - semantic boolean selection control.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_CHECKBOX_HPP
#define NANDINA_EXPERIMENT_WIDGET_CHECKBOX_HPP

#include "../reactive/event.hpp"
#include "../theme/checkbox_style.hpp"
#include "primitives/pressable.hpp"
#include "primitives/text.hpp"

#include <functional>
#include <memory>
#include <string>
#include <string_view>

namespace nandina::widget
{
    class Checkbox: public primitives::Pressable {
    public:
        explicit Checkbox(
            std::string label,
            bool checked = false,
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            std::string label,
            bool checked = false,
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Checkbox>;

        void set_label(std::string label);
        [[nodiscard]] auto label() const -> std::string_view;
        void set_checked(bool checked);
        [[nodiscard]] auto checked() const -> bool;
        void toggle();

        void set_on_change(std::function<void(bool)> callback);
        [[nodiscard]] auto checked_changed() const -> const reactive::Event<bool>&;

        void set_theme(theme::NanTheme theme);
        [[nodiscard]] auto theme_ref() const -> const theme::NanTheme&;
        [[nodiscard]] auto visual_state() const -> theme::CheckboxVisualState;
        [[nodiscard]] auto resolved_style() const -> theme::CheckboxStyle;

        void set_text_pipeline(primitives::TextPipeline pipeline);
        [[nodiscard]] auto text_pipeline() const -> primitives::TextPipeline;
        void apply_default_text_pipeline(const primitives::TextPipeline& pipeline) override;
        void apply_font_context(text::FontPipelineCache& context) override;
        void on_style_context_changed(const theme::ResolvedStyleContext& context) override;
        void on_theme_changed(const theme::ThemeManager& manager) override;
        void on_theme_context_removed() override;

        auto on_draw(render::DrawContext& context) -> void override;

    protected:
        [[nodiscard]] auto on_measure(scene::LayoutConstraints constraints)
            -> foundation::NanSize override;
        void on_click() override;
        void on_pressable_state_changed() override;
        [[nodiscard]] auto semantics_properties() const -> semantics::Properties override;
        auto on_semantics_action(const semantics::ActionRequest& request) -> bool override;

    private:
        void apply_metrics();
        void apply_text_style();

        primitives::Text text_;
        theme::NanTheme theme_;
        const theme::ThemeManager* theme_manager_ = nullptr;
        bool theme_explicit_ = false;
        bool checked_ = false;
        std::function<void(bool)> on_change_;
        reactive::Event<bool> checked_changed_;
    };
} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_CHECKBOX_HPP
