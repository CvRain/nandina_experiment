//
// theme/nan_style — component rules backed by theme tokens or literals.
//

#ifndef NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP

#include "button_style.hpp"
#include "text_field_style.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace nandina::theme
{
    enum class ColorToken {
        primary,
        on_primary,
        secondary,
        on_secondary,
        surface,
        on_surface,
        surface_variant,
        on_surface_variant,
        outline,
        outline_variant,
        error,
        on_error,
    };

    enum class ScalarToken {
        spacing_xs,
        spacing_sm,
        spacing_md,
        spacing_lg,
        spacing_xl,
        radius_sm,
        radius_md,
        radius_lg,
        radius_full,
        border_thin,
        border_medium,
        border_focus_ring,
        opacity_disabled,
        opacity_hover_overlay,
        opacity_pressed_overlay,
        typography_label_sm,
        typography_label_md,
        typography_label_lg,
    };

    template<typename T, typename Token>
    class ThemeValue {
    public:
        [[nodiscard]] static auto token(Token token) -> ThemeValue {
            return ThemeValue(token);
        }

        [[nodiscard]] static auto literal(T value) -> ThemeValue {
            return ThemeValue(std::move(value));
        }

        [[nodiscard]] auto value() const -> const std::variant<T, Token>& {
            return value_;
        }

    private:
        explicit ThemeValue(Token token): value_(token) {}
        explicit ThemeValue(T value): value_(std::move(value)) {}

        std::variant<T, Token> value_;
    };

    using ThemeColor = ThemeValue<NanColor, ColorToken>;
    using ThemeScalar = ThemeValue<float, ScalarToken>;

    [[nodiscard]] auto resolve_theme_color(const NanTheme& theme, const ThemeColor& value)
        -> NanColor;
    [[nodiscard]] auto resolve_theme_scalar(const NanTheme& theme, const ThemeScalar& value)
        -> float;

    struct ButtonRuleSelector {
        std::optional<ButtonTone> tone;
        std::optional<ButtonTreatment> treatment;
        std::optional<ButtonSize> size;
        std::optional<ButtonVisualState> state;

        [[nodiscard]] auto matches(
            ButtonTone current_tone,
            ButtonTreatment current_treatment,
            ButtonSize current_size,
            ButtonVisualState current_state
        ) const noexcept -> bool;
    };

    struct ButtonStyleRule {
        ButtonRuleSelector selector;
        std::optional<ThemeColor> background;
        std::optional<ThemeColor> foreground;
        std::optional<ThemeColor> border_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeScalar> border_width;
        std::optional<ThemeScalar> radius;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> height;
        std::optional<ThemeScalar> padding_x;
        std::optional<ThemeScalar> font_size;
    };

    struct TextFieldStyleRule {
        std::optional<TextFieldVisualState> state;
        std::optional<ThemeColor> background;
        std::optional<ThemeColor> foreground;
        std::optional<ThemeColor> placeholder;
        std::optional<ThemeColor> border_color;
        std::optional<ThemeColor> focus_ring_color;
        std::optional<ThemeColor> selection;
        std::optional<ThemeScalar> border_width;
        std::optional<ThemeScalar> radius;
        std::optional<ThemeScalar> focus_ring_width;
        std::optional<ThemeScalar> height;
        std::optional<ThemeScalar> padding_x;
        std::optional<ThemeScalar> font_size;
    };

    class NanStyle {
    public:
        virtual ~NanStyle() = default;

        void add_button_rule(ButtonStyleRule rule);
        [[nodiscard]] auto button_rules() const noexcept -> const std::vector<ButtonStyleRule>&;

        [[nodiscard]] virtual auto resolve_button(
            const NanTheme& theme,
            ButtonTone tone,
            ButtonTreatment treatment,
            ButtonSize size,
            ButtonVisualState state
        ) const -> ButtonStyle;

        void add_text_field_rule(TextFieldStyleRule rule);
        [[nodiscard]] auto text_field_rules() const noexcept
            -> const std::vector<TextFieldStyleRule>&;
        [[nodiscard]] virtual auto
        resolve_text_field(const NanTheme& theme, TextFieldVisualState state) const
            -> TextFieldStyle;

    private:
        std::vector<ButtonStyleRule> button_rules_;
        std::vector<TextFieldStyleRule> text_field_rules_;
    };

    [[nodiscard]] auto default_style() -> std::shared_ptr<const NanStyle>;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP
