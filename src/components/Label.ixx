module;
#include <concepts>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <type_traits>
#include <utility>

export module Nandina.Components.Label;

import Nandina.Core;
import Nandina.Reactive;

export namespace Nandina {

    struct LabelProps {
        const State<std::string>* text_signal = nullptr;
    };

    class Label final : public Component {
    public:
        static auto Create() -> std::unique_ptr<Label> {
            auto label = std::unique_ptr<Label>(new Label());
            label->set_hit_test_visible(false);
            return label;
        }

        static auto Create(LabelProps props) -> std::unique_ptr<Label> {
            auto label = std::unique_ptr<Label>(new Label());
            label->set_hit_test_visible(false);
            if (props.text_signal) {
                label->bind_text(*props.text_signal);
            }
            return label;
        }

        auto text(std::string t) -> Label& {
            text_.set(std::move(t));
            return *this;
        }

        // The bound state must outlive this Label (or at least its EffectScope).
        auto bind_text(const State<std::string>& state) -> Label& {
            scope_.add([this, &state] {
                text(state.get());
            });
            return *this;
        }

        template<typename T, typename F>
            requires std::copy_constructible<std::decay_t<F>>
                  && std::invocable<F&, const T&>
                  && std::convertible_to<std::invoke_result_t<F&, const T&>, std::string>
        // The bound state must outlive this Label (or at least its EffectScope).
        auto bind_text(const State<T>& state, F&& formatter) -> Label& {
            scope_.add([this, &state, formatter = std::forward<F>(formatter)] {
                text(std::invoke(formatter, state.get()));
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
