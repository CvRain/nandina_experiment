//
// widget/builtin_component_traits - authoring adapters for common built-in controls.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP

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
}

#endif // NANDINA_EXPERIMENT_WIDGET_BUILTIN_COMPONENT_TRAITS_HPP
