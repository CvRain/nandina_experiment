//
// widget/build_context - explicit authoring services for one build scope.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP

#include "../reactive/graph.hpp"
#include "../reactive/scope.hpp"
#include "../theme/theme_manager.hpp"
#include "authoring.hpp"

#include <string>
#include <utility>

namespace nandina::widget
{
    /// Lightweight, non-owning services passed through page and component construction.
    /// A derived region replaces only the reactive scope; graph and theme stay page-wide.
    class BuildContext {
    public:
        BuildContext(
            reactive::Graph& graph,
            reactive::ReactiveScope& scope,
            theme::ThemeManager& themes
        ) noexcept:
            graph_(&graph),
            scope_(&scope),
            themes_(&themes) {}

        [[nodiscard]] auto graph() const noexcept -> reactive::Graph& {
            return *graph_;
        }

        [[nodiscard]] auto scope() const noexcept -> reactive::ReactiveScope& {
            return *scope_;
        }

        [[nodiscard]] auto theme() const noexcept -> const theme::NanTheme& {
            return themes_->theme();
        }

        [[nodiscard]] auto theme_manager() const noexcept -> theme::ThemeManager& {
            return *themes_;
        }

        [[nodiscard]] auto with_scope(reactive::ReactiveScope& scope) const noexcept
            -> BuildContext {
            return BuildContext(*graph_, scope, *themes_);
        }

        [[nodiscard]] auto row() const -> authoring::NodeBuilder<Row> {
            return authoring::row();
        }

        [[nodiscard]] auto column() const -> authoring::NodeBuilder<Column> {
            return authoring::column();
        }

        [[nodiscard]] auto flex(LayoutAxis axis = LayoutAxis::horizontal) const
            -> authoring::NodeBuilder<Flex> {
            return authoring::flex(axis);
        }

        [[nodiscard]] auto padding(foundation::NanInsets insets) const
            -> authoring::NodeBuilder<Padding> {
            return authoring::padding(insets);
        }

        [[nodiscard]] auto center() const -> authoring::NodeBuilder<Center> {
            return authoring::center();
        }

        [[nodiscard]] auto expanded(int flex_factor = 1) const -> authoring::NodeBuilder<Expanded> {
            return authoring::expanded(flex_factor);
        }

        [[nodiscard]] auto flex_item(scene::LayoutFlexPolicy policy = {}) const
            -> authoring::NodeBuilder<FlexItem> {
            return authoring::flex_item(policy);
        }

        [[nodiscard]] auto scroll_view(ScrollAxis axis = ScrollAxis::vertical) const
            -> authoring::NodeBuilder<ScrollView> {
            return authoring::scroll_view(axis);
        }

        [[nodiscard]] auto grid(int columns = 2) const -> authoring::NodeBuilder<Grid> {
            return authoring::grid(columns);
        }

        [[nodiscard]] auto label(std::string text = {}) const -> authoring::NodeBuilder<Label> {
            return authoring::label(*graph_, std::move(text), themes_->theme());
        }

        [[nodiscard]] auto button(std::string text) const -> authoring::NodeBuilder<Button> {
            return authoring::button(std::move(text), themes_->theme());
        }

        [[nodiscard]] auto text_field(std::string value, std::string placeholder) const
            -> authoring::NodeBuilder<TextField> {
            return authoring::text_field(
                std::move(value),
                std::move(placeholder),
                themes_->theme()
            );
        }

    private:
        reactive::Graph* graph_;
        reactive::ReactiveScope* scope_;
        theme::ThemeManager* themes_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP
