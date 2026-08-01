//
// widget/build_context - explicit authoring services for one build scope.
//

#ifndef NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP
#define NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP

#include "../reactive/graph.hpp"
#include "../reactive/scope.hpp"
#include "../theme/theme_manager.hpp"
#include "authoring.hpp"

#include <concepts>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::widget
{
    namespace build_context_detail
    {
        template<typename Source>
        using list_item_t = typename std::remove_cvref_t<decltype(
            std::declval<Source&>().get()
        )>::value_type;
    } // namespace build_context_detail

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

        template<typename T, typename... Args>
        [[nodiscard]] auto signal(Args&&... args) const -> reactive::Signal<T>& {
            return scope_->signal<T>(std::forward<Args>(args)...);
        }

        template<typename T>
        [[nodiscard]] auto signal_value(T initial) const -> reactive::Signal<T>& {
            return scope_->signal_value(std::move(initial));
        }

        template<typename Fn>
            requires std::invocable<Fn>
        [[nodiscard]] auto computed(Fn&& fn) const
            -> reactive::Computed<std::invoke_result_t<Fn>>& {
            return scope_->computed(std::forward<Fn>(fn));
        }

        template<typename Fn>
            requires std::invocable<Fn>
        [[nodiscard]] auto effect(Fn&& fn) const -> reactive::Effect& {
            return scope_->effect(std::forward<Fn>(fn));
        }

        template<typename... Args, typename Handler>
        void connect(const reactive::Event<Args...>& event, Handler&& handler) const {
            scope_->connect(event, std::forward<Handler>(handler));
        }

        /// Bind a tracked source to an ordinary widget setter. The current build
        /// scope owns the effect, while a weak target prevents detached widgets
        /// from being kept alive solely by a binding.
        template<typename Node, typename Setter, typename Source>
            requires requires(Node& node, Setter setter, Source& source) {
                std::invoke(setter, node, source.get());
            }
        void bind(const std::shared_ptr<Node>& target, Setter setter, Source& source) const {
            scope_->effect(
                [weak = std::weak_ptr<Node>(target), setter = std::move(setter), &source] {
                    if (const auto current = weak.lock()) {
                        std::invoke(setter, *current, source.get());
                    }
                }
            );
        }

        /// Construct a custom component with its own reactive lifetime. The component
        /// receives the derived context and releases its subscriptions/effects before
        /// the concrete node is destroyed.
        template<typename Node, typename... Args>
            requires std::derived_from<Node, scene::NanNode>
            && std::constructible_from<Node, BuildContext, Args...>
        [[nodiscard]] auto make(Args&&... args) const -> authoring::NodeBuilder<Node> {
            auto scope = std::make_unique<reactive::ReactiveScope>(*graph_);
            auto component =
                std::unique_ptr<Node>(new Node(with_scope(*scope), std::forward<Args>(args)...));
            auto owned = std::shared_ptr<Node>(
                component.release(),
                [scope = std::move(scope)](Node* node) mutable {
                    scope->clear();
                    delete node;
                    scope.reset();
                }
            );
            return authoring::from(std::move(owned));
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

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        [[nodiscard]] auto label(Source& source) const -> authoring::NodeBuilder<Label> {
            auto result = label(std::string(source.get()));
            bind(result.build(), &Label::set_text, source);
            return result;
        }

        [[nodiscard]] auto button(std::string text) const -> authoring::NodeBuilder<Button> {
            return authoring::button(std::move(text), themes_->theme());
        }

        template<typename Source>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<const std::string&>;
            }
        [[nodiscard]] auto button(Source& source) const -> authoring::NodeBuilder<Button> {
            auto result = button(std::string(source.get()));
            bind(result.build(), &Button::set_text, source);
            return result;
        }

        [[nodiscard]] auto text_field(std::string value, std::string placeholder) const
            -> authoring::NodeBuilder<TextField> {
            return authoring::text_field(
                std::move(value),
                std::move(placeholder),
                themes_->theme()
            );
        }

        [[nodiscard]] auto text_field(reactive::Signal<std::string>& value, std::string placeholder)
            const -> authoring::NodeBuilder<TextField> {
            auto result = text_field(std::string(value.get()), std::move(placeholder));
            const auto field = result.build();
            bind(field, &TextField::set_value, value);
            connect(field->value_changed(), [&value](const std::string_view current) {
                if (value.peek() != current) {
                    value.set(std::string(current));
                }
            });
            return result;
        }

        template<typename Source, typename TrueFactory>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<bool>;
            } && std::invocable<TrueFactory&, BuildContext>
        [[nodiscard]] auto when(Source& source, TrueFactory&& when_true) const
            -> authoring::NodeBuilder<IfRegion<scene::NanControl>> {
            using Region = IfRegion<scene::NanControl>;
            auto region =
                Region::create(*graph_, make_branch_factory(std::forward<TrueFactory>(when_true)));
            region->bind(source);
            return authoring::from(std::move(region));
        }

        template<typename Source, typename TrueFactory, typename FalseFactory>
            requires requires(Source& source) {
                { source.get() } -> std::convertible_to<bool>;
            }
            && std::invocable<TrueFactory&, BuildContext> && std::invocable<
                FalseFactory&,
                BuildContext>
        [[nodiscard]] auto
        when(Source& source, TrueFactory&& when_true, FalseFactory&& when_false) const
            -> authoring::NodeBuilder<IfRegion<scene::NanControl>> {
            using Region = IfRegion<scene::NanControl>;
            auto region = Region::create(
                *graph_,
                make_branch_factory(std::forward<TrueFactory>(when_true)),
                make_branch_factory(std::forward<FalseFactory>(when_false))
            );
            region->bind(source);
            return authoring::from(std::move(region));
        }

        template<
            typename Source,
            typename KeyFunction,
            typename CreateFunction,
            typename UpdateFunction = std::nullptr_t>
            requires ListDataModelSource<Source, build_context_detail::list_item_t<Source>>
            && std::invocable<KeyFunction&, const build_context_detail::list_item_t<Source>&>
            && std::invocable<
                CreateFunction&,
                BuildContext,
                const build_context_detail::list_item_t<Source>&>
        [[nodiscard]] auto for_each(
            Source& source,
            KeyFunction&& key,
            CreateFunction&& create,
            UpdateFunction&& update = nullptr
        ) const {
            using Item = build_context_detail::list_item_t<Source>;
            using Key = std::remove_cvref_t<std::invoke_result_t<KeyFunction&, const Item&>>;
            using CreateResult = std::invoke_result_t<CreateFunction&, BuildContext, const Item&>;
            using NodePointer =
                decltype(authoring::detail::materialize(std::declval<CreateResult>()));
            using Node = typename NodePointer::element_type;
            static_assert(std::derived_from<Node, scene::NanControl>);
            using View = ListView<Item, Key, Node>;

            typename View::UpdateFunction update_node;
            if constexpr (!std::same_as<std::remove_cvref_t<UpdateFunction>, std::nullptr_t>) {
                static_assert(std::invocable<UpdateFunction&, Node&, const Item&>);
                update_node = [update = std::forward<UpdateFunction>(update)](
                                  Node& node,
                                  const Item& item
                              ) mutable { std::invoke(update, node, item); };
            }

            auto view = View::create(
                *graph_,
                [key = std::forward<KeyFunction>(key)](const Item& item) mutable {
                    return std::invoke(key, item);
                },
                [ui = *this, create = std::forward<CreateFunction>(create)](
                    reactive::ReactiveScope& scope,
                    const Item& item
                ) mutable {
                    return authoring::detail::materialize(
                        std::invoke(create, ui.with_scope(scope), item)
                    );
                },
                std::move(update_node)
            );
            view->set_model(source);
            return authoring::from(std::move(view));
        }

    private:
        template<typename Factory>
        [[nodiscard]] auto make_branch_factory(Factory&& factory) const
            -> typename IfRegion<scene::NanControl>::CreateFunction {
            return [ui = *this, factory = std::forward<Factory>(factory)](
                       reactive::ReactiveScope& scope
                   ) mutable -> std::shared_ptr<scene::NanControl> {
                auto node = authoring::detail::materialize(
                    std::invoke(factory, ui.with_scope(scope))
                );
                static_assert(
                    std::derived_from<typename decltype(node)::element_type, scene::NanControl>
                );
                return node;
            };
        }

        reactive::Graph* graph_;
        reactive::ReactiveScope* scope_;
        theme::ThemeManager* themes_;
    };

} // namespace nandina::widget

#endif // NANDINA_EXPERIMENT_WIDGET_BUILD_CONTEXT_HPP
