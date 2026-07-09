//
// reactive/scope — lifecycle owner for page/local reactive values.
//

#ifndef NANDINA_EXPERIMENT_REACTIVE_SCOPE_HPP
#define NANDINA_EXPERIMENT_REACTIVE_SCOPE_HPP

#include "computed.hpp"
#include "effect.hpp"
#include "signal.hpp"

#include <functional>
#include <memory>
#include <type_traits>
#include <utility>
#include <vector>

namespace nandina::reactive
{

    class ReactiveScope {
    public:
        explicit ReactiveScope(Graph& graph): graph_(&graph) {}

        ~ReactiveScope() {
            clear();
        }

        ReactiveScope(const ReactiveScope&) = delete;
        auto operator=(const ReactiveScope&) -> ReactiveScope& = delete;
        ReactiveScope(ReactiveScope&&) = delete;
        auto operator=(ReactiveScope&&) -> ReactiveScope& = delete;

        template<typename T, typename... Args>
        auto signal(Args&&... args) -> Signal<T>& {
            auto owned = std::make_unique<SignalHolder<T>>(*graph_, T(std::forward<Args>(args)...));
            auto* raw = &owned->value;
            signals_.push_back(std::move(owned));
            return *raw;
        }

        template<typename T>
        auto signal_value(T initial) -> Signal<T>& {
            auto owned = std::make_unique<SignalHolder<T>>(*graph_, std::move(initial));
            auto* raw = &owned->value;
            signals_.push_back(std::move(owned));
            return *raw;
        }

        template<typename Fn>
            requires std::invocable<Fn>
        auto computed(Fn&& fn) -> Computed<std::invoke_result_t<Fn>>& {
            auto* c = make_computed(*graph_, std::forward<Fn>(fn));
            computeds_.push_back([c] { c->dispose(); });
            return *c;
        }

        template<typename Fn>
            requires std::invocable<Fn>
        auto effect(Fn&& fn) -> Effect& {
            auto* e = make_effect(*graph_, std::forward<Fn>(fn));
            effects_.push_back(e);
            return *e;
        }

        void clear() {
            for (auto* e: effects_) {
                e->dispose();
            }
            effects_.clear();

            for (auto& dispose: computeds_) {
                dispose();
            }
            computeds_.clear();

            signals_.clear();
        }

        [[nodiscard]] auto signal_count() const -> std::size_t {
            return signals_.size();
        }

        [[nodiscard]] auto computed_count() const -> std::size_t {
            return computeds_.size();
        }

        [[nodiscard]] auto effect_count() const -> std::size_t {
            return effects_.size();
        }

    private:
        struct SignalHolderBase {
            virtual ~SignalHolderBase() = default;
        };

        template<typename T>
        struct SignalHolder final: SignalHolderBase {
            template<typename U>
            SignalHolder(Graph& graph, U&& initial): value(graph, std::forward<U>(initial)) {}

            Signal<T> value;
        };

        Graph* graph_;
        std::vector<std::unique_ptr<SignalHolderBase>> signals_;
        std::vector<std::function<void()>> computeds_;
        std::vector<Effect*> effects_;
    };

} // namespace nandina::reactive

#endif // NANDINA_EXPERIMENT_REACTIVE_SCOPE_HPP
