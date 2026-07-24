//
// widget/authoring - thin composition helpers over concrete widgets.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP
#define NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
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

} // namespace nandina::widget::authoring

#endif // NANDINA_EXPERIMENT_WIDGET_AUTHORING_HPP
