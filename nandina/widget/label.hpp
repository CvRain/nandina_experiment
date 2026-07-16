//
// widget/label — semantic text control with optional reactive text binding.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_LABEL_HPP
#define NANDINA_EXPERIMENT_WIDGET_LABEL_HPP

#include "../reactive/computed.hpp"
#include "../reactive/effect.hpp"
#include "../reactive/graph.hpp"
#include "../reactive/signal.hpp"
#include "../theme/theme.hpp"
#include "primitives/text.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <string>

namespace nandina::widget
{

    class Label: public primitives::Text {
    public:
        explicit Label(
            reactive::Graph& graph,
            std::string text = {},
            theme::NanTheme theme = theme::default_theme()
        );

        [[nodiscard]] static auto create(
            reactive::Graph& graph,
            std::string text = {},
            theme::NanTheme theme = theme::default_theme()
        ) -> std::shared_ptr<Label>;

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        void bind_text(Source& source) {
            binding_ = [&source](reactive::EffectScope& scope, primitives::Text& text) {
                text.text_property().bind(scope, source);
            };
            if (is_inside_tree()) {
                activate_binding();
            }
        }

    protected:
        void on_ready() override;
        void on_exit_tree() override;

    private:
        void activate_binding();

        reactive::EffectScope scope_;
        std::function<void(reactive::EffectScope&, primitives::Text&)> binding_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LABEL_HPP
