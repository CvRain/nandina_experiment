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

        void bind_text(reactive::Signal<std::string>& source);
        void bind_text(reactive::Computed<std::string>& source);

    protected:
        void on_ready() override;
        void on_exit_tree() override;

    private:
        reactive::EffectScope scope_;
        reactive::Signal<std::string>* signal_text_ = nullptr;
        reactive::Computed<std::string>* computed_text_ = nullptr;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_LABEL_HPP
