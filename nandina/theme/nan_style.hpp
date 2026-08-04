//
// theme/nan_style — component rules backed by theme tokens or literals.
//

#ifndef NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP
#define NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP

#include "button_style.hpp"
#include "visual_state.hpp"

#include <memory>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

namespace nandina::theme
{
    enum class ColorToken {
        background,
        on_background,
        primary,
        on_primary,
        secondary,
        on_secondary,
        tertiary,
        on_tertiary,
        surface,
        on_surface,
        surface_variant,
        on_surface_variant,
        outline,
        outline_variant,
        success,
        on_success,
        warning,
        on_warning,
        error,
        on_error,
        focus_ring,
        selection,
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

    using ThemeScalar = ThemeValue<float, ScalarToken>;

    // ─── 颜色表达式 ────────────────────────────────────────────────────────────
    //
    // ThemeColor 除 token / 字面量外，还支持浅层变换（with_alpha / transparent），
    // 使配方规则能表达「primary 的 hover 覆盖层」「surface 透明」等语义。

    /** 颜色变换的操作数：字面量或颜色 token。 */
    using ColorOperand = std::variant<NanColor, ColorToken>;

    /** 颜色变换类型。 */
    enum class ColorTransformOp {
        with_alpha,  // source.with_alpha(factor)
        transparent, // source.with_alpha(0)
    };

    /** 浅层颜色变换（操作数不嵌套表达式）。 */
    using ColorTransform = struct ColorTransform {
        ColorTransformOp op;
        ColorOperand source;
        ThemeScalar factor; // with_alpha 的 alpha；transparent 忽略
    };

    /** 颜色值：字面量 | 颜色 token | 浅层变换。 */
    class ThemeColor {
    public:
        [[nodiscard]] static auto token(ColorToken token) -> ThemeColor {
            return ThemeColor(token);
        }

        [[nodiscard]] static auto literal(NanColor value) -> ThemeColor {
            return ThemeColor(std::move(value));
        }

        /** @return source.with_alpha(factor) 的颜色表达式。 */
        [[nodiscard]] static auto with_alpha(ColorToken source, ThemeScalar factor) -> ThemeColor {
            return ThemeColor(ColorTransform {
                .op = ColorTransformOp::with_alpha,
                .source = source,
                .factor = std::move(factor),
            });
        }

        /** @return source.with_alpha(0)，即完全透明。 */
        [[nodiscard]] static auto transparent(ColorToken source) -> ThemeColor {
            return ThemeColor(ColorTransform {
                .op = ColorTransformOp::transparent,
                .source = source,
                .factor = ThemeScalar::literal(0.0F),
            });
        }

        [[nodiscard]] auto value() const
            -> const std::variant<NanColor, ColorToken, ColorTransform>& {
            return value_;
        }

    private:
        explicit ThemeColor(ColorToken token): value_(token) {}
        explicit ThemeColor(NanColor value): value_(std::move(value)) {}
        explicit ThemeColor(ColorTransform transform): value_(std::move(transform)) {}

        std::variant<NanColor, ColorToken, ColorTransform> value_;
    };

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

    private:
        std::vector<ButtonStyleRule> button_rules_;
        std::vector<TextFieldStyleRule> text_field_rules_;
    };

    [[nodiscard]] auto default_style() -> std::shared_ptr<const NanStyle>;

} // namespace nandina::theme

#endif // NANDINA_EXPERIMENT_THEME_NAN_STYLE_HPP
