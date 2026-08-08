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
#include "checkbox.hpp"
#include "grid.hpp"
#include "label.hpp"
#include "layout.hpp"
#include "list_view.hpp"
#include "scroll_view.hpp"
#include "slider.hpp"
#include "switch.hpp"
#include "text_field.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>

namespace nandina::widget::authoring
{
    // Authoring 代码可直接写 percent(50) / fill / content，同时底层类型仍归 scene 所有。
    using scene::content;
    using scene::fill;
    using scene::percent;

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

        template<typename Handler>
            requires requires(Node& node, Handler&& handler) {
                node.set_on_click(std::forward<Handler>(handler));
            }
        auto on_click(Handler&& handler) -> NodeBuilder& {
            node_->set_on_click(std::forward<Handler>(handler));
            return *this;
        }

        template<typename Handler>
            requires requires(Node& node, Handler&& handler) {
                node.set_on_submit(std::forward<Handler>(handler));
            }
        auto on_submit(Handler&& handler) -> NodeBuilder& {
            node_->set_on_submit(std::forward<Handler>(handler));
            return *this;
        }

        template<typename Handler>
            requires requires(Node& node, Handler&& handler) {
                node.set_on_change(std::forward<Handler>(handler));
            }
        auto on_change(Handler&& handler) -> NodeBuilder& {
            node_->set_on_change(std::forward<Handler>(handler));
            return *this;
        }

        auto checked(bool checked) -> NodeBuilder&
            requires requires(Node& node) { node.set_checked(checked); }
        {
            node_->set_checked(checked);
            return *this;
        }

        auto value(float value) -> NodeBuilder&
            requires requires(Node& node) { node.set_value(value); }
        {
            node_->set_value(value);
            return *this;
        }

        auto range(float minimum, float maximum) -> NodeBuilder&
            requires requires(Node& node) { node.set_range(minimum, maximum); }
        {
            node_->set_range(minimum, maximum);
            return *this;
        }

        auto step(float step) -> NodeBuilder&
            requires requires(Node& node) { node.set_step(step); }
        {
            node_->set_step(step);
            return *this;
        }

        auto tone(theme::ButtonTone tone) -> NodeBuilder&
            requires requires(Node& node) { node.set_tone(tone); }
        {
            node_->set_tone(tone);
            return *this;
        }

        auto treatment(theme::ButtonTreatment treatment) -> NodeBuilder&
            requires requires(Node& node) { node.set_treatment(treatment); }
        {
            node_->set_treatment(treatment);
            return *this;
        }

        auto gap(float gap) -> NodeBuilder&
            requires requires(Node& node) { node.set_gap(gap); }
        {
            node_->set_gap(gap);
            return *this;
        }

        auto width(float width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto width(scene::PercentLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto width(scene::FillLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto width(scene::ContentLength width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_width(width);
            return *this;
        }

        auto height(float height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto height(scene::PercentLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto height(scene::FillLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto height(scene::ContentLength height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_height(height);
            return *this;
        }

        auto min_width(float width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_min_width(width);
            return *this;
        }

        auto max_width(float width) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_max_width(width);
            return *this;
        }

        auto min_height(float height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_min_height(height);
            return *this;
        }

        auto max_height(float height) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_max_height(height);
            return *this;
        }

        auto aspect_ratio(float ratio) -> NodeBuilder&
            requires std::derived_from<Node, scene::NanControl>
        {
            node_->set_aspect_ratio(ratio);
            return *this;
        }

        auto cross_alignment(LayoutAlignment alignment) -> NodeBuilder&
            requires requires(Node& node) { node.set_cross_alignment(alignment); }
        {
            node_->set_cross_alignment(alignment);
            return *this;
        }

        auto font_size(float size) -> NodeBuilder&
            requires requires(Node& node) { node.set_font_size(size); }
        {
            node_->set_font_size(size);
            return *this;
        }

        auto color_token(theme::ColorToken token) -> NodeBuilder&
            requires requires(Node& node) { node.set_color_token(token); }
        {
            node_->set_color_token(token);
            return *this;
        }

        auto wheel_step(float step) -> NodeBuilder&
            requires requires(Node& node) { node.set_wheel_step(step); }
        {
            node_->set_wheel_step(step);
            return *this;
        }

        auto autofocus() -> NodeBuilder&
            requires requires(Node& node) { node.request_focus(); }
        {
            node_->request_focus();
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

    [[nodiscard]] inline auto grid(int columns = 2) -> NodeBuilder<Grid> {
        return make<Grid>(columns);
    }

    // ── 控件 ──

    [[nodiscard]] inline auto label(
        reactive::Graph& graph,
        std::string text = {},
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<Label> {
        return make<Label>(graph, std::move(text), theme);
    }

    [[nodiscard]] inline auto
    button(std::string text, theme::NanTheme theme = theme::default_theme())
        -> NodeBuilder<Button> {
        return make<Button>(std::move(text), theme);
    }

    [[nodiscard]] inline auto checkbox(
        std::string label,
        bool checked = false,
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<Checkbox> {
        return make<Checkbox>(std::move(label), checked, theme);
    }

    [[nodiscard]] inline auto slider(
        std::string label,
        float value = 0.0F,
        float minimum = 0.0F,
        float maximum = 1.0F,
        float step = 0.01F,
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<Slider> {
        return make<Slider>(std::move(label), value, minimum, maximum, step, theme);
    }

    // `switch` 是 C++ 关键字，工厂命名为 switch_control。
    [[nodiscard]] inline auto switch_control(
        std::string label,
        bool checked = false,
        theme::NanTheme theme = theme::default_theme()
    ) -> NodeBuilder<Switch> {
        return make<Switch>(std::move(label), checked, theme);
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
