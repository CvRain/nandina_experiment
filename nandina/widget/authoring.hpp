//
// widget/authoring - thin composition helpers over concrete widgets.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP
#define NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP

#include "../foundation/geometry.hpp"
#include "../reactive/graph.hpp"
#include "../scene/control.hpp"
#include "../theme/theme.hpp"
#include "button.hpp"
#include "label.hpp"
#include "layout.hpp"
#include "scroll_view.hpp"
#include "text_field.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::widget::authoring
{
    template<typename Node>
    class NodeBuilder;

    namespace detail
    {
        template<typename Node>
        [[nodiscard]] auto materialize(std::shared_ptr<Node> node) -> std::shared_ptr<Node> {
            return node;
        }

        template<typename Node>
        [[nodiscard]] auto materialize(const NodeBuilder<Node>& builder) -> std::shared_ptr<Node> {
            return builder.build();
        }

        template<typename Parent, typename Child>
        concept AddableChild = requires(Parent& parent, Child&& child) {
            parent.add(materialize(std::forward<Child>(child)));
        };

        template<typename Parent, typename Child>
        concept SettableChild = requires(Parent& parent, Child&& child) {
            parent.set_child(materialize(std::forward<Child>(child)));
        };
    } // namespace detail

    template<typename Node>
    class NodeBuilder {
    public:
        explicit NodeBuilder(std::shared_ptr<Node> node): node_(std::move(node)) {
            if (!node_) {
                throw std::invalid_argument("NodeBuilder node cannot be null");
            }
        }

        template<typename... Configure>
            requires(std::invocable<Configure, Node&> && ...)
        auto configure(Configure&&... configure) -> NodeBuilder& {
            (static_cast<void>(std::invoke(std::forward<Configure>(configure), *node_)), ...);
            return *this;
        }

        template<typename... Child>
            requires(detail::AddableChild<Node, Child> && ...)
        auto children(Child&&... child) -> NodeBuilder& {
            (static_cast<void>(node_->add(detail::materialize(std::forward<Child>(child)))), ...);
            return *this;
        }

        template<typename Child>
            requires detail::SettableChild<Node, Child>
        auto child(Child&& child) -> NodeBuilder& {
            node_->set_child(detail::materialize(std::forward<Child>(child)));
            return *this;
        }

        auto expose(std::shared_ptr<Node>& target) -> NodeBuilder& {
            target = node_;
            return *this;
        }

        [[nodiscard]] auto get() noexcept -> Node& {
            return *node_;
        }

        [[nodiscard]] auto get() const noexcept -> const Node& {
            return *node_;
        }

        [[nodiscard]] auto build() const -> std::shared_ptr<Node> {
            return node_;
        }

    private:
        std::shared_ptr<Node> node_;
    };

    template<typename Node, typename... Args>
        requires std::constructible_from<Node, Args...>
    [[nodiscard]] auto make(Args&&... args) -> NodeBuilder<Node> {
        return NodeBuilder<Node>(std::make_shared<Node>(std::forward<Args>(args)...));
    }

    template<typename Node>
    [[nodiscard]] auto from(std::shared_ptr<Node> node) -> NodeBuilder<Node> {
        return NodeBuilder<Node>(std::move(node));
    }

    // ──自由函数工厂：用简洁的名称创建常见控件 ────────────────
    // 这些是 make<T>(args...) 的语义别名，不引入新的对象模型。

    // ── 布局 ──

    [[nodiscard]] inline auto row() -> NodeBuilder<Row> {
        return make<Row>();
    }

    [[nodiscard]] inline auto column() -> NodeBuilder<Column> {
        return make<Column>();
    }

    [[nodiscard]] inline auto flex(LayoutAxis axis = LayoutAxis::horizontal) -> NodeBuilder<Flex> {
        return make<Flex>(axis);
    }

    [[nodiscard]] inline auto padding(foundation::NanInsets insets) -> NodeBuilder<Padding> {
        return make<Padding>(insets);
    }

    [[nodiscard]] inline auto center() -> NodeBuilder<Center> {
        return make<Center>();
    }

    [[nodiscard]] inline auto expanded(int flex_factor = 1) -> NodeBuilder<Expanded> {
        return make<Expanded>(flex_factor);
    }

    [[nodiscard]] inline auto flex_item(scene::LayoutFlexPolicy policy = {})
        -> NodeBuilder<FlexItem> {
        return make<FlexItem>(policy);
    }

    [[nodiscard]] inline auto scroll_view(ScrollAxis axis = ScrollAxis::vertical)
        -> NodeBuilder<ScrollView> {
        return make<ScrollView>(axis);
    }

    // ── 控件 ──

    [[nodiscard]] inline auto label(
        reactive::Graph& graph,
        std::string text = {},
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<Label> {
        return make<Label>(graph, std::move(text), theme);
    }

    [[nodiscard]] inline auto button(
        std::string text,
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<Button> {
        return make<Button>(std::move(text), theme);
    }

    [[nodiscard]] inline auto text_field(
        std::string value,
        std::string placeholder,
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<TextField> {
        return make<TextField>(std::move(value), std::move(placeholder), theme);
    }

} // namespace nandina::widget::authoring

#endif // NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP
