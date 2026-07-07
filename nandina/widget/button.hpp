//
// widget/button — first semantic control built from tone/treatment tokens.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP

#include "../theme/button_style.hpp"
#include "primitives/pressable.hpp"

#include <string>
#include <string_view>
#include <memory>

namespace nandina::widget
{

    class Button: public primitives::Pressable {
    public:
        explicit Button(std::string text, theme::NanTheme theme = theme::default_theme());

        [[nodiscard]] static auto create(std::string text, theme::NanTheme theme = theme::default_theme())
            -> std::shared_ptr<Button>;

        void set_text(std::string text);
        [[nodiscard]] auto text() const -> std::string_view;

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
        void on_pressable_state_changed() override;

    private:
        void apply_metrics();

        std::string text_;
        theme::NanTheme theme_;
        theme::ButtonTone tone_ = theme::ButtonTone::primary;
        theme::ButtonTreatment treatment_ = theme::ButtonTreatment::filled;
        theme::ButtonSize size_ = theme::ButtonSize::medium;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUTTON_HPP
