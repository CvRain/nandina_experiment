//
// widget/builtin_component_traits - authoring adapters for common built-in controls.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP

#include "build_context.hpp"
#include "badge.hpp"
#include "button.hpp"
#include "card.hpp"
#include "checkbox.hpp"
#include "label.hpp"
#include "slider.hpp"
#include "switch.hpp"
#include "text_field.hpp"

namespace nandina::widget
{
    template<>
    struct ComponentTraits<Label> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text = {})
            -> authoring::NodeBuilder<Label> {
            return authoring::make<Label>(ui.graph(), std::move(text), ui.theme());
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        [[nodiscard]] static auto make(const BuildContext& ui, Source& source)
            -> authoring::NodeBuilder<Label> {
            auto result = make(ui, std::string(source.get()));
            ui.bind(result.build(), &Label::set_text, source);
            return result;
        }
    };

    template<>
    struct ComponentTraits<Button> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text)
            -> authoring::NodeBuilder<Button> {
            return authoring::make<Button>(std::move(text), ui.theme());
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        [[nodiscard]] static auto make(const BuildContext& ui, Source& source)
            -> authoring::NodeBuilder<Button> {
            auto result = make(ui, std::string(source.get()));
            ui.bind(result.build(), &Button::set_text, source);
            return result;
        }
    };

    template<>
    struct ComponentTraits<Checkbox> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            const bool checked = false
        ) -> authoring::NodeBuilder<Checkbox> {
            return authoring::make<Checkbox>(std::move(label), checked, ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<bool>& checked,
            std::string label
        ) -> authoring::NodeBuilder<Checkbox> {
            auto result = make(ui, std::move(label), checked.get());
            const auto control = result.build();
            ui.bind(control, &Checkbox::set_checked, checked);
            ui.connect(control->checked_changed(), [&checked](const bool current) {
                if (checked.peek() != current) {
                    checked.set(current);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Switch> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            const bool checked = false
        ) -> authoring::NodeBuilder<Switch> {
            return authoring::make<Switch>(std::move(label), checked, ui.theme());
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<bool>& checked,
            std::string label
        ) -> authoring::NodeBuilder<Switch> {
            auto result = make(ui, std::move(label), checked.get());
            const auto control = result.build();
            ui.bind(control, &Switch::set_checked, checked);
            ui.connect(control->checked_changed(), [&checked](const bool current) {
                if (checked.peek() != current) {
                    checked.set(current);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Slider> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string label,
            const float value = 0.0F,
            const float minimum = 0.0F,
            const float maximum = 1.0F,
            const float step = 0.01F
        ) -> authoring::NodeBuilder<Slider> {
            return authoring::make<Slider>(
                std::move(label), value, minimum, maximum, step, ui.theme()
            );
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<float>& value,
            std::string label,
            const float minimum = 0.0F,
            const float maximum = 1.0F,
            const float step = 0.01F
        ) -> authoring::NodeBuilder<Slider> {
            auto result = make(ui, std::move(label), value.get(), minimum, maximum, step);
            const auto control = result.build();
            ui.bind(control, &Slider::set_value, value);
            ui.connect(control->value_changed(), [&value](const float current) {
                if (std::abs(value.peek() - current) > foundation::nan_epsilon) {
                    value.set(current);
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<TextField> {
        [[nodiscard]] static auto make(
            const BuildContext& ui,
            std::string value,
            std::string placeholder
        ) -> authoring::NodeBuilder<TextField> {
            return authoring::make<TextField>(
                std::move(value), std::move(placeholder), ui.theme()
            );
        }

        [[nodiscard]] static auto make(
            const BuildContext& ui,
            reactive::Signal<std::string>& value,
            std::string placeholder
        ) -> authoring::NodeBuilder<TextField> {
            auto result = make(ui, std::string(value.get()), std::move(placeholder));
            const auto field = result.build();
            ui.bind(field, &TextField::set_value, value);
            ui.connect(field->value_changed(), [&value](const std::string_view current) {
                if (value.peek() != current) {
                    value.set(std::string(current));
                }
            });
            return result;
        }
    };

    template<>
    struct ComponentTraits<Badge> {
        [[nodiscard]] static auto make(const BuildContext& ui, std::string text)
            -> authoring::NodeBuilder<Badge> {
            return authoring::make<Badge>(std::move(text), ui.theme());
        }
    };

    template<>
    struct ComponentTraits<Card> {
        [[nodiscard]] static auto make(const BuildContext& ui) -> authoring::NodeBuilder<Card> {
            return authoring::make<Card>(ui.theme());
        }
    };
}

#endif // NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
