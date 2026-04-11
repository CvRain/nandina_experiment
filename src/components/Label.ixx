module;
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <optional>
#include <string>
#include <type_traits>
#include <utility>

export module Nandina.Components.Label;

import Nandina.Core;
import Nandina.Reactive;

export namespace Nandina {

    struct LabelProps {
        // Component inputs should prefer Prop/ReadState so the child only sees
        // a read-only dependency. Direct State binding remains available for
        // component-internal wiring where write access still belongs locally.
        std::optional<Prop<std::string>> text;
    };

    class Label final : public Component {
    public:
        static auto Create() -> std::unique_ptr<Label> {
            auto label = std::unique_ptr<Label>(new Label());
            label->set_hit_test_visible(false);
            return label;
        }

        static auto Create(Prop<std::string> text) -> std::unique_ptr<Label> {
            return Create(LabelProps{.text = std::optional<Prop<std::string>>{std::move(text)}});
        }

        static auto Create(LabelProps props) -> std::unique_ptr<Label> {
            auto label = std::unique_ptr<Label>(new Label());
            label->set_hit_test_visible(false);
            if (props.text) {
                label->bind_text(std::move(*props.text));
            }
            return label;
        }

        auto text(std::string t) -> Label& {
            text_.set(std::move(t));
            return *this;
        }

        auto bind_text(Prop<std::string> prop) -> Label& {
            if (auto* state = prop.state_ptr()) {
                return bind_text(*state);
            }
            return text(std::string{prop.get()});
        }

        auto bind_text(ReadState<std::string> state) -> Label& {
            return bind_text(Prop<std::string>{state});
        }

        // SAFETY: The bound state must remain valid for the lifetime of this Label's
        // EffectScope. Binding to a local state that gets destroyed is undefined behavior;
        // prefer state owned by a parent component or page that outlives this Label.
        // Violations can show up as crashes or stale text during later state updates.
        auto bind_text(const State<std::string>& state) -> Label& {
            scope_.add([text_state = &text_, state = std::cref(state)] {
                const auto& source = state.get();
                text_state->set(source.get());
            });
            return *this;
        }

        template<typename T, typename F>
            requires std::copy_constructible<std::decay_t<F>>
                  && std::invocable<F&, const T&>
                  && std::convertible_to<std::invoke_result_t<F&, const T&>, std::string>
        auto bind_text(Prop<T> prop, F&& formatter) -> Label& {
            if (auto* state = prop.state_ptr()) {
                return bind_text(*state, std::forward<F>(formatter));
            }
            return text(std::invoke(std::forward<F>(formatter), prop.get()));
        }

        template<typename T, typename F>
            requires std::copy_constructible<std::decay_t<F>>
                  && std::invocable<F&, const T&>
                  && std::convertible_to<std::invoke_result_t<F&, const T&>, std::string>
        auto bind_text(ReadState<T> state, F&& formatter) -> Label& {
            return bind_text(Prop<T>{state}, std::forward<F>(formatter));
        }

        template<typename T, typename F>
            requires std::copy_constructible<std::decay_t<F>>
                  && std::invocable<F&, const T&>
                  && std::convertible_to<std::invoke_result_t<F&, const T&>, std::string>
        // SAFETY: The bound state must remain valid for the lifetime of this Label's
        // EffectScope. Binding to a local state that gets destroyed is undefined behavior;
        // prefer state owned by a parent component or page that outlives this Label.
        // Violations can show up as crashes or stale text during later state updates.
        auto bind_text(const State<T>& state, F&& formatter) -> Label& {
            scope_.add([text_state = &text_, state = std::cref(state), formatter = std::forward<F>(formatter)] {
                const auto& source = state.get();
                text_state->set(std::invoke(formatter, source.get()));
            });
            return *this;
        }

        auto font_size(float size) -> Label& {
            font_size_ = size;
            mark_dirty();
            return *this;
        }

        auto text_color(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) -> Label& {
            text_r_ = r; text_g_ = g; text_b_ = b; text_a_ = a;
            mark_dirty();
            return *this;
        }

        [[nodiscard]] auto get_text()      const -> const std::string& { return text_.get(); }
        [[nodiscard]] auto get_font_size() const -> float              { return font_size_; }
        [[nodiscard]] auto text_r()        const -> std::uint8_t       { return text_r_; }
        [[nodiscard]] auto text_g()        const -> std::uint8_t       { return text_g_; }
        [[nodiscard]] auto text_b()        const -> std::uint8_t       { return text_b_; }
        [[nodiscard]] auto text_a()        const -> std::uint8_t       { return text_a_; }
        [[nodiscard]] auto text_content() const noexcept -> std::string_view override { return text_.get(); }
        [[nodiscard]] auto text_color() const noexcept -> Color override { return {text_r_, text_g_, text_b_, text_a_}; }
        [[nodiscard]] auto text_font_size() const noexcept -> float override { return font_size_; }

    private:
        Label() {
            set_background(0, 0, 0, 0);
            // Track text_ as a reactive dependency: when text_ changes, mark this widget dirty for re-render.
            scope_.add([this]{ (void)text_(); mark_dirty(); });
        }

        State<std::string> text_{ std::string{""} };
        float font_size_    = 14.0f;
        std::uint8_t text_r_ = 0;
        std::uint8_t text_g_ = 0;
        std::uint8_t text_b_ = 0;
        std::uint8_t text_a_ = 255;
    };

} // export namespace Nandina
